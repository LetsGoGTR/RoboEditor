#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H
#pragma once
#include <QString>
#include <QStringList>

namespace FileCompressor
{
    bool    createTarGz(const QStringList &files, const QString &tarPath);
    bool    extractTarGz(const QString &tarPath, const QString &destDir);
    QString getUniqueFilePath(const QString &originalPath);
}  // namespace FileCompressor

#endif  // FILECOMPRESSOR_H
