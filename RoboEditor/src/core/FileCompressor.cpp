#include "FileCompressor.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <archive.h>
#include <archive_entry.h>

namespace FileCompressor
{

    static inline QByteArray enc(const QString &s)
    {
        // OS 로컬 인코딩으로 변환 (Windows에서 한글 경로 안전)
        return QFile::encodeName(s);
    }

    bool createTarGz(const QStringList &inputs, const QString &tarGzPath)
    {
        struct archive *a = archive_write_new();
        if (!a) {
            qWarning() << "[createTarGz] archive_write_new failed";
            return false;
        }

        // 🔹 gzip 필터 추가 (tar.gz로 만들기)
        if (archive_write_add_filter_gzip(a) != ARCHIVE_OK) {
            qWarning() << "[createTarGz] add_filter_gzip failed:" << archive_error_string(a);
            archive_write_free(a);
            return false;
        }

        // 🔹 tar 포맷 지정 (POSIX 호환)
        if (archive_write_set_format_pax_restricted(a) != ARCHIVE_OK) {
            qWarning() << "[createTarGz] set_format failed:" << archive_error_string(a);
            archive_write_free(a);
            return false;
        }

        // 🔹 출력 파일 열기
#if defined(Q_OS_WIN)
        if (archive_write_open_filename(a, QFile::encodeName(tarGzPath).constData()) !=
            ARCHIVE_OK) {
#else
        if (archive_write_open_filename(a, tarGzPath.toUtf8().constData()) != ARCHIVE_OK) {
#endif
            qWarning() << "[createTarGz] open_filename failed:" << tarGzPath
                       << archive_error_string(a);
            archive_write_free(a);
            return false;
        }

        // 🔹 재귀적 파일 추가 함수
        std::function<bool(const QString &, const QString &)> addPath =
                [&](const QString &path, const QString &root) -> bool {
            QFileInfo     info(path);
            const QString relPath = QDir(root).relativeFilePath(path);

            if (info.isDir()) {
                struct archive_entry *dirEntry = archive_entry_new();
                archive_entry_set_pathname(dirEntry, QFile::encodeName(relPath).constData());
                archive_entry_set_filetype(dirEntry, AE_IFDIR);
                archive_entry_set_perm(dirEntry, 0755);
                archive_write_header(a, dirEntry);
                archive_entry_free(dirEntry);

                QDir dir(path);
                for (const QFileInfo &child :
                     dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries))
                    addPath(child.filePath(), root);
                return true;
            }

            if (!info.isFile())
                return true;

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                return false;

            struct archive_entry *entry = archive_entry_new();
            archive_entry_set_pathname(entry, QFile::encodeName(relPath).constData());
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            archive_entry_set_size(entry, file.size());
            archive_write_header(a, entry);

            QByteArray buf(128 * 1024, 0);
            while (!file.atEnd()) {
                qint64 n = file.read(buf.data(), buf.size());
                if (n > 0)
                    archive_write_data(a, buf.constData(), size_t(n));
            }

            archive_entry_free(entry);
            return true;
        };

        bool overallOK = true;
        for (const QString &in : inputs) {
            QFileInfo fi(in);
            if (!fi.exists()) {
                qWarning() << "[createTarGz] input not found:" << in;
                overallOK = false;
                continue;
            }

            const QString root = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
            overallOK          = addPath(fi.absoluteFilePath(), root) && overallOK;
        }

        // 🔹 닫기 및 정리
        if (archive_write_close(a) != ARCHIVE_OK) {
            qWarning() << "[createTarGz] close failed:" << archive_error_string(a);
            overallOK = false;
        }
        archive_write_free(a);

        QFileInfo out(tarGzPath);
        if (!out.exists() || out.size() == 0) {
            qWarning() << "[createTarGz] output missing or empty:" << tarGzPath;
            overallOK = false;
        } else {
            qDebug() << "[createTarGz] OK:" << tarGzPath << "size =" << out.size() << "bytes";
        }

        return overallOK;
    }

    bool extractTarGz(const QString &tarPath, const QString &destDir)
    {
        QDir dir;
        if (!dir.exists(destDir)) {
            if (!dir.mkpath(destDir)) {
                qWarning() << "디렉토리 생성 실패:" << destDir;
                return false;
            }
        }

        struct archive       *a = archive_read_new();
        struct archive_entry *entry;

        // 모든 필터를 내장 구현으로 지원
        archive_read_support_filter_all(a);

        // 모든 포맷 지원
        archive_read_support_format_all(a);

        int r = archive_read_open_filename(a, tarPath.toUtf8().constData(), 10240);
        if (r != ARCHIVE_OK) {
            qWarning() << "tar 파일 열기 실패:" << tarPath;
            qWarning() << "Error:" << archive_error_string(a);
            archive_read_free(a);
            return false;
        }

        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            const char *currentFile = archive_entry_pathname(entry);
            QString     outputPath  = QDir(destDir).filePath(QString::fromUtf8(currentFile));

            qDebug() << "압축 해제 중:" << currentFile << "->" << outputPath;

            mode_t fileType = archive_entry_filetype(entry);

            if (fileType == AE_IFDIR) {
                if (!dir.mkpath(outputPath)) {
                    qWarning() << "디렉토리 생성 실패:" << outputPath;
                    continue;
                }
            } else if (fileType == AE_IFREG) {
                // 상위 디렉토리 생성
                QFileInfo fileInfo(outputPath);
                if (!dir.mkpath(fileInfo.absolutePath())) {
                    qWarning() << "상위 디렉토리 생성 실패:" << fileInfo.absolutePath();
                    continue;
                }

                QFile outFile(outputPath);
                if (!outFile.open(QIODevice::WriteOnly)) {
                    qWarning() << "파일 쓰기 실패:" << outputPath;
                    continue;
                }

                const void *buff;
                size_t      size;
                int64_t     offset;

                while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                    outFile.write(static_cast<const char *>(buff), size);
                }

                outFile.close();

                mode_t perm = archive_entry_perm(entry);
                QFile::setPermissions(outputPath, QFileDevice::Permission(perm & 0777));
            }
        }

        archive_read_close(a);
        archive_read_free(a);

        return true;
    }

}  // namespace FileCompressor
