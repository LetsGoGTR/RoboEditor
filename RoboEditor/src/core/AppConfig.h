#ifndef APPCONFIG_H
#define APPCONFIG_H
#pragma once

#include <QString>
#include <QDir>
#include <QCoreApplication>

class AppConfig {
  public:
    // 실행 파일 위치 기준 'backup' 폴더 경로 반환 (예: D:/MyApp/backup)
    static QString getBackupPath() {
        QString path = QDir(QCoreApplication::applicationDirPath()).filePath("backup");

        // 폴더가 없으면 생성
        QDir dir(path);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        return path;
    }
};

#endif  // APPCONFIG_H
