#pragma once

// Helpers for the Qt 4 tests (Qt 4 has no QTemporaryDir, QSignalSpy::wait() or
// QTest::qWaitFor()).

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QTimer>

inline QString s(const char *latin1)
{
    return QString::fromLatin1(latin1);
}

// Runs a real event loop for `ms` milliseconds. Unlike QTest::qWait(), this also
// deletes objects whose deleteLater() was called outside any event loop (Qt 4 only
// does that when control returns to an event loop).
inline void runEventLoop(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}

// Waits until `spy` has recorded one more signal, or `timeout` ms have passed.
inline bool waitForSignal(QSignalSpy &spy, int timeout = 5000)
{
    const int before = spy.count();
    QElapsedTimer timer;
    timer.start();
    while (spy.count() == before && timer.elapsed() < timeout) {
        runEventLoop(10);
    }
    return spy.count() > before;
}

// Waits until `condition()` is true (a functor or function), or `timeout` ms have passed.
template<typename Condition>
bool waitFor(Condition condition, int timeout = 2000)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < timeout) {
        runEventLoop(10);
    }
    return condition();
}

// A fresh, empty directory in the temp directory, removed (with its files) on destruction.
class TemporaryDir
{
public:
    TemporaryDir()
    {
        static int counter = 0;
        const QString name = QString::fromLatin1("ttime-test-%1-%2")
                                 .arg(QCoreApplication::applicationPid())
                                 .arg(++counter);
        m_path = QDir::temp().filePath(name);
        removeFiles();
        m_valid = QDir().mkpath(m_path);
    }
    ~TemporaryDir()
    {
        removeFiles();
        QDir().rmdir(m_path);
    }

    bool isValid() const { return m_valid; }
    QString path() const { return m_path; }
    QString filePath(const QString &name) const { return QDir(m_path).filePath(name); }

private:
    Q_DISABLE_COPY(TemporaryDir)

    void removeFiles()
    {
        QDir dir(m_path);
        foreach (const QString &file, dir.entryList(QDir::Files | QDir::Hidden)) {
            dir.remove(file);
        }
    }

    QString m_path;
    bool m_valid;
};
