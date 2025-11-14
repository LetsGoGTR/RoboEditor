#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H
#pragma once
#include <QDialog>
#include <QElapsedTimer>

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

class ProgressDialog : public QDialog {
    Q_OBJECT
  public:
    explicit ProgressDialog(QWidget *parent = nullptr);

    void setTotalCount(int total);
    void setCurrentIndex(int index);
    void setSerialNumber(const QString &sn);
    void setProgress(int percent);
    void setStatusText(const QString &text);
    void setFinishedMode(bool finished);
    void reset();

  signals:
    void cancelRequested();

  private slots:
    void updateElapsedTime();

  private:
    QLabel *serialLabel_;
    QLabel *countLabel_;
    QLabel *timeLabel_;
    QLabel *statusLabel_;
    QProgressBar *progressBar_;
    QPushButton *cancelBtn_;

    QElapsedTimer elapsed_;
    QTimer *timer_;
    int totalCount_ = 0;
    int currentIndex_ = 0;
};

#endif  // PROGRESSDIALOG_H
