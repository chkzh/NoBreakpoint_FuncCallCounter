#include "FuncWatcherDialog.h"
#include "Bridge.h"
#include <QtWidgets/qsizepolicy>
//#include <QVBoxLayout>
#include <QResizeEvent>
//#include <QFileDialog>
#include <QAction>
#include <QDebug>
#include <QFileDialog>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")// 该指令仅支持VS环境
#endif


FuncWatcherDialog::FuncWatcherDialog(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //this->changed_tab_height = 0;
    //this->changed_tab_width = 0;

    this->old_tab_height = ui.tabWidget->height();
    this->is_first_resize = true;

    init();
}

FuncWatcherDialog::~FuncWatcherDialog()
{
    UpdateStatusClock.stop();
    RefreshClock.stop();
}

void FuncWatcherDialog::init()
{
    this->setMinimumSize(this->size());
    this->setMaximumWidth(this->size().width());
    ui.radioButton_inj->setChecked(true);
    ui.checkBox_lvl1->setChecked(true);
    ui.checkBox_lvl2->setChecked(true);
    ui.checkBox_Iat->setChecked(true);

    ui.comboBox_DataType->addItem("次数（近期）");
    ui.comboBox_DataType->addItem("频率（近期）");
    ui.comboBox_DataType->addItem("次数（累计）");

    ui.comboBox_FilterType->addItem("（无）");
    ui.comboBox_FilterType->addItem("非零");
    ui.comboBox_FilterType->addItem("范围");
    ui.comboBox_FilterType->addItem("特定值");
    ui.comboBox_FilterType->addItem("特定倍数");

    ui.comboBox_SortType->addItem("由高到低");
    ui.comboBox_SortType->addItem("由低到高");
    ui.comboBox_SortType->addItem("接近特定值");
    ui.comboBox_SortType->addItem("接近特定倍数");
    
    connect(ui.actionOpenFile, SIGNAL(triggered()), this, SLOT(OpenFile()));
    connect(ui.actionStart, SIGNAL(triggered()), this, SLOT(StartProc()));
    connect(ui.actionStop, SIGNAL(triggered()), this, SLOT(StopProc()));
    connect(ui.actionDetach, SIGNAL(triggered()), this, SLOT(Detach()));
    connect(ui.actionRestart, SIGNAL(triggered()), this, SLOT(Restart()));

    RefreshClock.setInterval(2000);
    connect(&RefreshClock, SIGNAL(timeout()), this, SLOT(RefreshData()));

    UpdateStatusClock.setInterval(1000);
    connect(&UpdateStatusClock, SIGNAL(timeout()), this, SLOT(UpdateStatus()));
    UpdateStatusClock.start();

    ui.tableWidget->setHorizontalHeaderLabels(QStringList() << "数值" << "函数名" << "导入方" << "导出方");
   // ui.tableWidget->

    std::string str1 = "1145.14";
    std::string str2 = "ExAllocateFromPagedPool";
    std::string str3 = "Not My Fault.exe";
    std::string str4 = "Ntoskrnl.exe";
    _InsertRecord(str1, str2, str3, str4);
    ui.tableWidget->resizeRowsToContents();
    ui.tableWidget->resizeColumnsToContents();

    QRegularExpression reg_int("[0-9]{1,8}");
    QRegularExpression reg_float("^(\\d{1,8})?(\\.\\d{1,2})?$");
    QRegularExpression reg_count("[0-9]{1,2}");

    this->IntLimiter = new QRegularExpressionValidator(reg_int);
    this->FloatLimiter = new QRegularExpressionValidator(reg_float);
    this->CountLimiter = new QRegularExpressionValidator(reg_count);


    ui.lineEdit_MaxCount->setValidator(this->CountLimiter);
    ui.lineEdit_ExtraParam->setValidator(this->FloatLimiter);
    ui.lineEdit_ExtraParam1->setValidator(this->FloatLimiter);
    ui.lineEdit_ExtraParam2->setValidator(this->FloatLimiter);

    //
    // 初始化默认设置
    //
    this->SetInjectMode(true);

    CurPrintCount = "28";
    ui.lineEdit_MaxCount->setText(CurPrintCount);
    ui.comboBox_DataType->setCurrentIndex(DT_RECENT_COUNT);
    ui.comboBox_FilterType->setCurrentIndex(FT_NON_ZERO_VALUE);
    ui.comboBox_SortType->setCurrentIndex(ST_TOP_DOWN);
    ApplyDataPickSettings();
}

void FuncWatcherDialog::SetDebugMode(bool is_on)
{
    if (!is_on)
        return;

    ui.checkBox_Gpa->setChecked(true);
    ui.checkBox_Gpa->setDisabled(false);
    ui.pushButton_Undo->setDisabled(true);
    ui.pushButton_Install->setDisabled(true);

    //【后端】
    bkSwapMode(PROC_MODE::PM_DBG);

    ui.statusBar->showMessage("已将模式设为：调试");
}

void FuncWatcherDialog::UninstallHooks()
{
    bkUninstall();
    ui.statusBar->showMessage("已卸载钩子");
}

void FuncWatcherDialog::InstallAndStart()
{
    if (flag_IsHooked)
    {
        ui.statusBar->showMessage(" * 请先卸载原有钩子！");
        return;
    }

    bkInstall();
    ui.statusBar->showMessage("已安装钩子");
}

void FuncWatcherDialog::ChangeFilterType(int index)
{
    switch (index)
    {
    case 0:
    case 1:
        ui.lineEdit_ExtraParam1->setDisabled(true);
        ui.lineEdit_ExtraParam2->setDisabled(true);
        break;
    case 2:
        ui.lineEdit_ExtraParam1->setDisabled(false);
        ui.lineEdit_ExtraParam2->setDisabled(false);
        break;
    case 3:
    case 4:
        ui.lineEdit_ExtraParam1->setDisabled(false);
        ui.lineEdit_ExtraParam2->setDisabled(true);
        break;
    }
}

void FuncWatcherDialog::ChangeSortType(int index)
{
    switch (index)
    {
    case 0:
    case 1:
        ui.lineEdit_ExtraParam->setDisabled(true);
        break;
    case 2:
    case 3:
        ui.lineEdit_ExtraParam->setDisabled(false);
        break;
    }
}

void FuncWatcherDialog::ApplyDataPickSettings()
{
    if (!bkSetDataType(ui.comboBox_DataType->currentIndex()))
    {
        ui.statusBar->showMessage("* * * 数据类型无效");
        ui.comboBox_DataType->setCurrentIndex(CurDataTypeIndex);
    }
    else
        CurDataTypeIndex = ui.comboBox_DataType->currentIndex();

    QString param1 = ui.lineEdit_ExtraParam1->text();
    QString param2 = ui.lineEdit_ExtraParam2->text();
    if (!bkSetFilterType(ui.comboBox_FilterType->currentIndex(), param1, param2))
    {
        ui.statusBar->showMessage("* * *过滤类型无效");
        ui.comboBox_FilterType->setCurrentIndex(CurFilterTypeIndex);
        ui.lineEdit_ExtraParam1->setText(CurExtraParam1);
        ui.lineEdit_ExtraParam2->setText(CurExtraParam2);
    }
    else {
        CurExtraParam1 = param1;
        CurExtraParam2 = param2;
        CurFilterTypeIndex = ui.comboBox_FilterType->currentIndex();
    }

    QString param = ui.lineEdit_ExtraParam->text();
    QString count = ui.lineEdit_MaxCount->text();
    if (!bkSetSortType(ui.comboBox_SortType->currentIndex(), param, count))
    {
        ui.statusBar->showMessage("* * *排序类型无效");
        ui.comboBox_FilterType->setCurrentIndex(CurFilterTypeIndex);
        ui.lineEdit_ExtraParam->setText(CurExtraParam);
        ui.lineEdit_MaxCount->setText(CurPrintCount);
    }
    else {
        CurPrintCount = count;
        CurExtraParam = param;
        CurSortTypeIndex = ui.comboBox_SortType->currentIndex();
    }

    ui.statusBar->showMessage("已更改数据提取设置", 3);
}

void FuncWatcherDialog::SetLoopMode(bool is_on)
{
    QString reply;
    if (is_on)
    {
        RefreshClock.start();
        reply = "循环已启动，3秒/次";
    }
    else
    {
        RefreshClock.stop();
        reply = "循环已关闭";
    }
    ui.statusBar->showMessage(reply);
}

void FuncWatcherDialog::RefreshData()
{
    ui.textEdit_MoreInfo->clear();
    ui.tableWidget->setRowCount(0);

    if (!flag_IsProcessing)
        return;

    bkRefresh();

    auto ite = CurToShowRecords.begin();
    while (ite != CurToShowRecords.end())
    {
        std::string val_str;
        if (CurDataTypeIndex == DT_RECENT_FREQUENCY)
        {
            val_str = std::to_string(ite->val.frq);
            val_str = val_str.substr(0, val_str.find(".") + 3);
        }
        else
            val_str = std::to_string(ite->val.cnt);

        _InsertRecord(val_str, ite->func_name, ite->importer_name, ite->exporter_name);

        ite++;
    }

    //if (CurToShowRecords.size()) {
    //    ui.tableWidget->resizeRowsToContents();
    //    ui.tableWidget->resizeColumnsToContents();
    //}

    QString str = "检测到 " + QString::number(CurChangedCount) + " 条调用记录";

    ui.statusBar->showMessage(str);
}

void FuncWatcherDialog::GetMoreInfo(int index, int index_2)
{
    QString more_info;
    bkQurey(index, more_info);
    more_info = ui.tableWidget->item(index, index_2)->text() + "\n" + more_info;

    ui.textEdit_MoreInfo->setText(more_info);

    ui.statusBar->showMessage("已显示补充信息");
}

void FuncWatcherDialog::ShowHelp()
{
    ui.statusBar->showMessage("* 请查看作者的补充文档 *");
}

void FuncWatcherDialog::ExecuteCommand()
{
    ui.statusBar->showMessage("Executing a command..");
    ui.textEdit_reply->setText("暂不支持  > _ <;");
}

void FuncWatcherDialog::OpenFile()
{
    QFileDialog* fileDialog = new QFileDialog(this, "选择文件", ".", "可执行文件(*.exe)");
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setViewMode(QFileDialog::Detail);
    if (fileDialog->exec() == QDialog::Accepted)
    {
        QString fileName = fileDialog->selectedFiles().first();
        ui.statusBar->showMessage(fileName, 3);
        
        CurExePath = fileName.toLocal8Bit().toStdString();
        bkSelectFile();
    }

    UpdateStatus();
}

void FuncWatcherDialog::StartProc()
{
    QString reply;
    if (bkStart())
    {
        reply = "已创建进程";
        this->_LockAndGetOnStart();
    }
    else
        reply = "* * * 创建进程失败！";

    UpdateStatus();
    ui.statusBar->showMessage(reply);
}

void FuncWatcherDialog::Restart()
{
    QString reply;
    if (bkRestart())
    {
        reply = "已重启进程";
        _LockAndGetOnStart();
    }
    else
        reply = "* * * 重启进程失败！";

    UpdateStatus();
    ui.statusBar->showMessage(reply);
}

void FuncWatcherDialog::Detach()
{
    QString reply;
    if (bkDetach())
    {
        reply = "进程已脱离";
        this->_UnlockOnClose();
    }
    else
        reply = "* * * 进程脱离失败！";

    ui.statusBar->showMessage(reply);
}

void FuncWatcherDialog::StopProc()
{
    bkClose();
    _UnlockOnClose();
    if (CurMode == PM_INJECT)
        SetInjectMode(true);
    else
        SetDebugMode(true);

    UpdateStatus();
    ui.statusBar->showMessage("进程已终止");
}

void FuncWatcherDialog::UpdateStatus()
{
    QString title = QString::fromStdString(CurExeName);

    DWORD status = UsingProc->checkProcessStatus();
    if (!(status & PS_PROCESS_AVAILABLE) && flag_IsProcessing)
        title = "（已无效）" + title;
    else if (flag_IsProcessing)
        title = "（已启动）" + title;
    else
        title = "（未启动）" + title;

    this->setWindowTitle(title);
}

void FuncWatcherDialog::_InsertRecord(std::string& dat, std::string& func_name, std::string& importer, std::string& exporter)
{
    int row = ui.tableWidget->rowCount();
    ui.tableWidget->setRowCount(row + 1);
    ui.tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(dat)));
    ui.tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(func_name)));
    ui.tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromLatin1(importer)));
    ui.tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLatin1(exporter)));
}

void FuncWatcherDialog::_LockAndGetOnStart()
{
    ui.radioButton_dbg->setDisabled(true);
    ui.radioButton_inj->setDisabled(true);

    ui.checkBox_Gpa->setDisabled(true);
    ui.checkBox_Iat->setDisabled(true);

    if (CurMode != PM_INJECT) {
        ui.checkBox_lvl1->setDisabled(true);
        ui.checkBox_lvl2->setDisabled(true);
        ui.checkBox_lvl3->setDisabled(true);
        ui.checkBox_lvl4->setDisabled(true);

        //ui.pushButton_Install->setDisabled(true);
        //ui.pushButton_Undo->setDisabled(true);
    }

    DWORD typ_flag = 0;
    if (ui.checkBox_Gpa->isChecked())
        typ_flag |= HT_GET_PROC_ADDRESS;
    if (ui.checkBox_Iat->isChecked())
        typ_flag |= HT_IMPORT_ADDRESS_TABLE;

    DWORD tar_flag = 0;
    if (ui.checkBox_lvl1->isChecked())
        tar_flag |= HM_EXE;
    if (ui.checkBox_lvl2->isChecked())
        tar_flag |= HM_EXE_DIRECTORY_DLLS;
    if (ui.checkBox_lvl3->isChecked())
        tar_flag |= HM_OTHER_DIRECTORY_DLLS;
    if (ui.checkBox_lvl4->isChecked())
        tar_flag |= HM_SYSTEM_DIRECTORY_DLLS;

    UsingProc->setHookTarget(tar_flag);
    UsingProc->setHookType(typ_flag);
}

void FuncWatcherDialog::_UnlockOnClose()
{
    ui.radioButton_dbg->setDisabled(false);
    ui.radioButton_inj->setDisabled(false);

    ui.checkBox_Gpa->setDisabled(false);
    ui.checkBox_Iat->setDisabled(false);

    if (CurMode != PM_INJECT) {
        ui.checkBox_lvl1->setDisabled(false);
        ui.checkBox_lvl2->setDisabled(false);
        ui.checkBox_lvl3->setDisabled(false);
        ui.checkBox_lvl4->setDisabled(false);

        //ui.pushButton_Install->setDisabled(false);
        //ui.pushButton_Undo->setDisabled(false);
    }

    if (CurMode == PROC_MODE::PM_DBG)
        SetDebugMode(true);
    else
        SetInjectMode(true);
}

void FuncWatcherDialog::resizeEvent(QResizeEvent* _event)
{


    //old_tab_height = ui.tabWidget->height();


    ui.tabWidget->resize(_event->size().width() - 20, _event->size().height() - 100);
    int changed_tab_height = ui.tabWidget->height() - old_tab_height;
    int changed_tab_width = 0;
    qDebug() << "new:  " << ui.tabWidget->height() << "old: " << old_tab_height  << " = " << changed_tab_height;


    old_tab_height = ui.tabWidget->height();

    if (is_first_resize)
    {
        is_first_resize = false;
        return;
    }


    //修改控件的大小
    ADJUST_SIZE(ui.tableWidget);
    ADJUST_SIZE(ui.textEdit_reply);

    //修改控件的位置
    ADJUST_POS(ui.lineEdit_cmd);
    ADJUST_POS(ui.pushButton_help);
    ADJUST_POS(ui.label_8);
    ADJUST_POS(ui.pushButton_refresh);
    ADJUST_POS(ui.radioButton_loop);
    ADJUST_POS(ui.textEdit_MoreInfo);
    ADJUST_POS(ui.pushButton_Undo);
    ADJUST_POS(ui.pushButton_Install);

}

void FuncWatcherDialog::SetInjectMode(bool is_on)
{
    if (!is_on)
        return;

    ui.checkBox_Gpa->setChecked(false);
    ui.checkBox_Gpa->setDisabled(true);
    ui.pushButton_Undo->setDisabled(false);
    ui.pushButton_Install->setDisabled(false);

    //【后端】
    bkSwapMode(PM_INJECT);

    ui.statusBar->showMessage("已改为注入模式");
}