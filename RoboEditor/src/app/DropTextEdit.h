#ifndef DROPTEXTEDIT_H
#define DROPTEXTEDIT_H
#pragma once
#include <QPlainTextEdit>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

// 텍스트 비교 영역(왼쪽/오른쪽)에 쓰는 드롭 가능한 에디터
class DropTextEdit : public QPlainTextEdit {
    Q_OBJECT
  public:
    explicit DropTextEdit(QWidget* parent = nullptr);

    QString lastLoadedPath() const { return lastPath_; }

    // 왼쪽/오른쪽 어떤 용도로도 받을 수 있게 플래그를 켜둘 수 있음
    void setAcceptAsLeft(bool v)  { acceptAsLeft_  = v; }
    void setAcceptAsRight(bool v) { acceptAsRight_ = v; }

    // 파일 로드 후 텍스트 채울 때 호출
    void setLoadedText(const QString& text, const QString& srcPath);

  signals:
    void fileDroppedToLeft(const QString& path);
    void fileDroppedToRight(const QString& path);

  protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

  private:
    bool acceptAsLeft_  = false;
    bool acceptAsRight_ = false;
    QString lastPath_;
};

#endif  // DROPTEXTEDIT_H
