#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include "ui_FuncWatcher.h"
#include "ui_MainWatchDialog.h"


#define ADJUST_SIZE(x) x->resize(x->size().width(), x->size().height() + changed_tab_height)
#define ADJUST_POS(s) s->move(s->pos().x(), s->pos().y() + changed_tab_height)

class FuncWatcherDialog : public QMainWindow
{
    Q_OBJECT

public:
    FuncWatcherDialog(QWidget *parent = nullptr);
    ~FuncWatcherDialog();


private:
    Ui::MainWatchDialog ui;

 /*   int tab_width_delta;
    int tab_height_delta;*/
    //int changed_tab_height;
    //int changed_tab_width;

    QValidator* IntLimiter;
    QValidator* FloatLimiter;
    QValidator* CountLimiter;

    QTimer RefreshClock;
    QTimer UpdateStatusClock;

    int old_tab_height;
    bool is_first_resize;

    void init();

private slots:
    //监视设置
    void SetInjectMode(bool is_on);
    void SetDebugMode(bool is_on);

    void UninstallHooks();
    void InstallAndStart();
    void ChangeFilterType(int index);
    void ChangeSortType(int index);
    void ApplyDataPickSettings();

    //数据展示
    void SetLoopMode(bool);
    void RefreshData();
    void GetMoreInfo(int index, int index_2);

    //高级
    void ShowHelp();
    void ExecuteCommand();

    //图标
    void OpenFile();
    void StartProc();
    void Restart();
    void Detach();
    void StopProc();

    //更新状态
    void UpdateStatus();

private:
    //杂项函数
    void _InsertRecord(std::string& dat, std::string& func_name, std::string& importer, std::string& exporter);
    void _LockAndGetOnStart();
    void _UnlockOnClose();
    //void InsertRecord(float count, std::string& func_name, std::string& importer, std::string& exporter);

protected:
    void resizeEvent(QResizeEvent* event) override;
};
