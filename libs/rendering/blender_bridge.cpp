#include "blender_bridge.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <algorithm>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
namespace lmx {
BlenderBridge::BlenderBridge(QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS;
    });
    childJob = CreateJobObjectW(nullptr, nullptr);
    if (childJob) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(childJob, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
            CloseHandle(childJob);
            childJob = nullptr;
        }
    }
    connect(&process, &QProcess::started, this, [this] {
        if (childJob) {
            auto child = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE,
                                     static_cast<DWORD>(process.processId()));
            if (child) {
                if (!AssignProcessToJobObject(childJob, child))
                    emit output("Não foi possível vincular a interrupção automática do processo.\n");
                CloseHandle(child);
            }
        }
    });
#else
    process.setChildProcessModifier([] {
        setpriority(PRIO_PROCESS, 0, 10);
        const auto parent = getppid();
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() != parent)
            raise(SIGTERM);
    });
#endif
    connect(&process, &QProcess::readyReadStandardOutput, this,
            [this] { emit output(QString::fromUtf8(process.readAllStandardOutput())); });
    connect(&process, &QProcess::readyReadStandardError, this,
            [this] { emit output(QString::fromUtf8(process.readAllStandardError())); });
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        emit output(process.errorString() + '\n');
        if (error == QProcess::FailedToStart && !reported) {
            reported = true;
            emit finished(-1, false);
        }
    });
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                emit output(QString::fromUtf8(process.readAllStandardOutput()));
                emit output(QString::fromUtf8(process.readAllStandardError()));
                if (!reported) {
                    reported = true;
                    emit finished(code, status == QProcess::NormalExit);
                }
            });
}
BlenderBridge::~BlenderBridge() {
    cancel();
    if (process.state() != QProcess::NotRunning)
        process.waitForFinished(3000);
#ifdef Q_OS_WIN
    if (childJob)
        CloseHandle(childJob);
#endif
}
QString BlenderBridge::findExecutable(const QString &preferred) {
    if (!preferred.isEmpty() && QFileInfo(preferred).isExecutable())
        return preferred;
    auto path = QStandardPaths::findExecutable("blender");
    if (!path.isEmpty())
        return path;
#ifdef Q_OS_WIN
    for (const auto &base :
         {qEnvironmentVariable("ProgramFiles"), qEnvironmentVariable("ProgramFiles(x86)")}) {
        QDir directory(base + "/Blender Foundation");
        for (const auto &name : directory.entryList({"Blender*"}, QDir::Dirs | QDir::NoDotAndDotDot,
                                                    QDir::Name | QDir::Reversed)) {
            auto executable = directory.filePath(name + "/blender.exe");
            if (QFileInfo(executable).isExecutable())
                return executable;
        }
    }
#endif
    return {};
}
void BlenderBridge::start(const QString &executable, const QString &script, const QStringList &arguments) {
    if (busy())
        throw std::runtime_error("Já existe um processo Blender em execução");
    reported = false;
    process.setProgram(executable);
    QStringList args{"--background",
                     "--factory-startup",
                     "--threads",
                     QString::number(std::clamp(QThread::idealThreadCount() - 2, 1, 8)),
                     "--python-exit-code",
                     "1",
                     "--python",
                     script,
                     "--"};
    args.append(arguments);
    process.setArguments(args);
    process.start();
}
void BlenderBridge::cancel() {
    if (busy())
        process.kill();
}
bool BlenderBridge::busy() const {
    return process.state() != QProcess::NotRunning;
}
} // namespace lmx
