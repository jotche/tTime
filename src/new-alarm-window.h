#pragma once

#include "alarm-spec.h"

#include <QScopedPointer>
#include <QWidget>

struct AppConfig;

namespace Ui {
class NewAlarmWindow;
}

// Small input window where the user types e.g. "5m green tea".
class NewAlarmWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NewAlarmWindow(const AppConfig &config, QWidget *parent = 0);
    ~NewAlarmWindow();

    // Pre-fills the input (e.g. with an unparsable command line) and
    // highlights it as invalid if it cannot be parsed.
    void setText(const QString &text);

signals:
    void alarmRequested(const AlarmSpec &spec);
    // Emitted when the user closes the window (not when it is hidden programmatically).
    void dismissed();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void submit();
    void onTextEdited();

private:
    void setInputValid(bool valid);

    QScopedPointer<Ui::NewAlarmWindow> ui;
};
