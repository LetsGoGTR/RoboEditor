#ifndef FILETYPEHELPER_H
#define FILETYPEHELPER_H
#pragma once
#include <QString>

class FileTypeHelper
{
  public:
    enum class Type {
        Text,
        Yaml,
        Python,
        Unknown
    };

    // 경로(혹은 파일명)에서 확장자를 보고 Type 판정
    static Type detect(const QString &path);

    // 경로 기준 이름 ("yaml", "python", "text", "unknown")
    static QString typeName(const QString &path);

    // Type 기준 이름
    static QString typeName(Type type);

    // 두 타입이 비교 가능한지 여부
    static bool isCompatible(Type left, Type right);
};

#endif  // FILETYPEHELPER_H
