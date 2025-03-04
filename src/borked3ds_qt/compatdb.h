// Copyright 2017 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QFutureWatcher>
#include <QWizard>

namespace Ui {
class CompatDB;
}

class CompatDB : public QWizard {
    Q_OBJECT

public:
    explicit CompatDB(QWidget* parent = nullptr);
    ~CompatDB();

private:
    QFutureWatcher<bool> testcase_watcher;

    std::unique_ptr<Ui::CompatDB> ui;

    void Submit();
    void OnTestcaseSubmitted();
    void EnableNext();
};
