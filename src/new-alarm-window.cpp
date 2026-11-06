#include "new-alarm-window.h"

#include "app-config.h"
#include "ui_new-alarm-window.h"

#include <QCloseEvent>
#include <QToolTip>

namespace {

const QString usageHint = QStringLiteral("Enter a duration and an optional message, e.g.\n"
                                         "  5m green tea\n"
                                         "  1h 30m meeting\n"
                                         "  90s");

} // namespace

NewAlarmWindow::NewAlarmWindow(const AppConfig &config, QWidget *parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::NewAlarmWindow>())
{
    ui->setupUi(this);
    setWindowTitle(tr("New alarm"));
    // Windows shows a window's own icon on the taskbar; without one it falls back to
    // the (blue) icon compiled into tTime.exe, not QApplication::windowIcon().
    setWindowIcon(themedAppIcon(config.theme));
    ui->lineEdit->setPlaceholderText(tr("e.g. 5m green tea"));
    ui->lineEdit->setToolTip(usageHint);
    ui->lineEdit->setClearButtonEnabled(true);

    connect(ui->startAlarmButton, &QPushButton::clicked, this, &NewAlarmWindow::submit);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &NewAlarmWindow::submit);
    connect(ui->lineEdit, &QLineEdit::textEdited, this, [this] { setInputValid(true); });
}

NewAlarmWindow::~NewAlarmWindow() = default;

void NewAlarmWindow::setText(const QString &text)
{
    ui->lineEdit->setText(text);
    setInputValid(text.isEmpty() || parseAlarmSpec(text).has_value());
}

void NewAlarmWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit dismissed();
}

void NewAlarmWindow::submit()
{
    const auto spec = parseAlarmSpec(ui->lineEdit->text());
    if (!spec) {
        setInputValid(false);
        QToolTip::showText(ui->lineEdit->mapToGlobal(QPoint(0, ui->lineEdit->height())), usageHint, ui->lineEdit);
        return;
    }
    ui->lineEdit->clear();
    emit alarmRequested(*spec);
}

void NewAlarmWindow::setInputValid(bool valid)
{
    ui->lineEdit->setStyleSheet(valid ? QString() : QStringLiteral("QLineEdit { border: 1px solid #d9534f; }"));
}
