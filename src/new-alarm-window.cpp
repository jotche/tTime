#include "new-alarm-window.h"

#include "app-config.h"
#include "ui_new-alarm-window.h"

#include <QCloseEvent>
#include <QToolTip>

namespace {

QString usageHint()
{
    return QLatin1String("Enter a duration and an optional message, e.g.\n"
                         "  5m green tea\n"
                         "  1h 30m meeting\n"
                         "  90s");
}

} // namespace

NewAlarmWindow::NewAlarmWindow(const AppConfig &config, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewAlarmWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("New alarm"));
    // Windows shows a window's own icon on the taskbar; without one it falls back to
    // the (blue) icon compiled into tTime.exe, not QApplication::windowIcon().
    setWindowIcon(themedAppIcon(config.theme));
    ui->lineEdit->setPlaceholderText(tr("e.g. 5m green tea"));
    ui->lineEdit->setToolTip(usageHint());

    connect(ui->startAlarmButton, SIGNAL(clicked()), this, SLOT(submit()));
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(submit()));
    connect(ui->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onTextEdited()));
}

NewAlarmWindow::~NewAlarmWindow()
{
}

void NewAlarmWindow::setText(const QString &text)
{
    ui->lineEdit->setText(text);
    setInputValid(text.isEmpty() || parseAlarmSpec(text));
}

void NewAlarmWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit dismissed();
}

void NewAlarmWindow::submit()
{
    AlarmSpec spec;
    if (!parseAlarmSpec(ui->lineEdit->text(), &spec)) {
        setInputValid(false);
        QToolTip::showText(ui->lineEdit->mapToGlobal(QPoint(0, ui->lineEdit->height())), usageHint(), ui->lineEdit);
        return;
    }
    ui->lineEdit->clear();
    emit alarmRequested(spec);
}

void NewAlarmWindow::onTextEdited()
{
    setInputValid(true);
}

void NewAlarmWindow::setInputValid(bool valid)
{
    ui->lineEdit->setStyleSheet(valid ? QString() : QLatin1String("QLineEdit { border: 1px solid #d9534f; }"));
}
