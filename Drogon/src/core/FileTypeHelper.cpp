#include "FileTypeHelper.h"
#include <QFileInfo>

FileTypeHelper::Type FileTypeHelper::detect(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();

    // DiffService 에 맞춘 확장자 매핑
    if (ext == "yaml" || ext == "yml" || ext == "pts")
        return Type::Yaml;

    if (ext == "py" || ext == "srl" || ext == "sbp")
        return Type::Python;

    if (!ext.isEmpty())
        return Type::Text;

    return Type::Unknown;
}

QString FileTypeHelper::typeName(const QString &path)
{
    return typeName(detect(path));
}

QString FileTypeHelper::typeName(Type type)
{
    switch (type) {
    case Type::Yaml:   return "yaml";
    case Type::Python: return "python";
    case Type::Text:   return "text";
    default:           return "unknown";
    }
}

bool FileTypeHelper::isCompatible(Type left, Type right)
{
    // 완전히 같은 타입이면 OK
    if (left == right)
        return true;

    // 둘 다 일반 텍스트면 OK
    if (left == Type::Text && right == Type::Text)
        return true;

    // 그 외는 일단 허용하지 않음
    return false;
}
