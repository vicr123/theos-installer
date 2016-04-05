#include "worker.h"

Worker::Worker() {
}

// --- DECONSTRUCTOR ---
Worker::~Worker() {
}

void Worker::process() {

    int exitCode = QProcess::execute("ping", QStringList() << "-c 1" << "archlinux.org");
    emit finished(exitCode);
}
