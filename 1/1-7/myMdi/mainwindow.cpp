#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mdichild.h"
#include <QMdiSubWindow>
#include <QFileDialog>
#include <QSignalMapper>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    actionSeparator = new QAction(this);
    actionSeparator->setSeparator(true);
    updateMenus();
    connect(ui->mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateMenus()));

    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));

    updateWindowMenu();
    connect(ui->menuW, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    readSettings();
    initWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionNew_triggered()
{
    MdiChild *child = createMdiChild();
    child->newFile();
    child->show();
}

MdiChild *MainWindow::createMdiChild()
{
    MdiChild *child = new MdiChild;
    ui->mdiArea->addSubWindow(child);

    connect(child, SIGNAL(copyAvailable(bool)), ui->actionCut, SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)), ui->actionCopy, SLOT(setEnabled(bool)));
    connect(child->document(), SIGNAL(undoAvailable(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect(child->document(), SIGNAL(redoAvailable(bool)), ui->actionRedo, SLOT(setEnabled(bool)));

    connect(child, SIGNAL(cursorPositionChanged()), this, SLOT(showTextRowAndCol()));

    return child;
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != 0);
    ui->actionSave->setEnabled(hasMdiChild);
    ui->actionSaveAs->setEnabled(hasMdiChild);
    ui->actionPaste->setEnabled(hasMdiChild);
    ui->actionClose->setEnabled(hasMdiChild);
    ui->actionCloseAll->setEnabled(hasMdiChild);
    ui->actionTile->setEnabled(hasMdiChild);
    ui->actionCascade->setEnabled(hasMdiChild);
    ui->actionNext->setEnabled(hasMdiChild);
    ui->actionPrevious->setEnabled(hasMdiChild);

    actionSeparator->setVisible(hasMdiChild);

    bool hasSelection = (activeMdiChild() && activeMdiChild()->textCursor().hasSelection());
    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);

    ui->actionUndo->setEnabled(activeMdiChild() && activeMdiChild()->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(activeMdiChild() && activeMdiChild()->document()->isRedoAvailable());
}

MdiChild *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = ui->mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return 0;
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        QMdiSubWindow *existing = findMdiChild(fileName);
        if (existing)
        {
            ui->mdiArea->setActiveSubWindow(existing);
            return;
        }

        MdiChild *child = createMdiChild();
        if (child->loadFile(fileName))
        {
            ui->statusBar->showMessage(tr("??????????????????"), 2000);
            child->show();
        }
        else
        {
            child->close();
        }
    }
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList())
    {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if (mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::updateWindowMenu()
{
    ui->menuW->clear();
    ui->menuW->addAction(ui->actionClose);
    ui->menuW->addAction(ui->actionCloseAll);
    ui->menuW->addSeparator();
    ui->menuW->addAction(ui->actionTile);
    ui->menuW->addAction(ui->actionCascade);
    ui->menuW->addSeparator();
    ui->menuW->addAction(ui->actionNext);
    ui->menuW->addAction(ui->actionPrevious);
    ui->menuW->addAction(actionSeparator);

    QList<QMdiSubWindow *>windows = ui->mdiArea->subWindowList();
    actionSeparator->setVisible(!windows.isEmpty());

    for (int i=0; i<windows.size(); ++i)
    {
        MdiChild *child = qobject_cast<MdiChild *>(windows.at(i)->widget());
        QString text;
        if (i<9)
        {
            text = tr("&%1 %2").arg(i + 1).arg(child->userFriendlyCurrentFile());
        }
        else
        {
            text = tr("%1 %2").arg(i + 1).arg(child->userFriendlyCurrentFile());
        }
        QAction *action = ui->menuW->addAction(text);
        action->setCheckable(true);

        action->setChecked(child == activeMdiChild());

        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));

        windowMapper->setMapping(action, windows.at(i));
    }
}

void MainWindow::on_actionSave_triggered()
{
    if (activeMdiChild() && activeMdiChild()->save())
        ui->statusBar->showMessage(tr("??????????????????"), 2000);
}

void MainWindow::on_actionSaveAs_triggered()
{
    if (activeMdiChild() && activeMdiChild()->saveAs())
        ui->statusBar->showMessage(tr("??????????????????"), 2000);
}

void MainWindow::on_actionUndo_triggered()
{
    if (activeMdiChild()) activeMdiChild()->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    if (activeMdiChild()) activeMdiChild()->redo();
}

void MainWindow::on_actionCut_triggered()
{
    if (activeMdiChild()) activeMdiChild()->cut();
}

void MainWindow::on_actionCopy_triggered()
{
    if (activeMdiChild()) activeMdiChild()->copy();
}

void MainWindow::on_actionPaste_triggered()
{
    if (activeMdiChild()) activeMdiChild()->paste();
}

void MainWindow::on_actionClose_triggered()
{
    ui->mdiArea->closeActiveSubWindow();
}

void MainWindow::on_actionCloseAll_triggered()
{
    ui->mdiArea->closeAllSubWindows();
}

void MainWindow::on_actionTile_triggered()
{
    ui->mdiArea->tileSubWindows();
}

void MainWindow::on_actionCascade_triggered()
{
    ui->mdiArea->cascadeSubWindows();
}

void MainWindow::on_actionNext_triggered()
{
    ui->mdiArea->activateNextSubWindow();
}

void MainWindow::on_actionPrevious_triggered()
{
    ui->mdiArea->activatePreviousSubWindow();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("???????????????"), tr("??????????????????????????????www.yafeilinux.com"));
}

void MainWindow::on_actionAboutQt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_actionExit_triggered()
{
    qApp->closeAllWindows();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ui->mdiArea->closeAllSubWindows();
    if (ui->mdiArea->currentSubWindow())
    {
        event->ignore();
    }
    else
    {
        writeSettings();
        event->accept();
    }
}

void MainWindow::writeSettings()
{
    QSettings settings("yafeilinux", "myMdi");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

void MainWindow::readSettings()
{
    QSettings settings("yafeilinux", "myMdi");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void MainWindow::showTextRowAndCol()
{
    if (activeMdiChild())
    {
        int rowNum = activeMdiChild()->textCursor().blockNumber() + 1;
        int colNum = activeMdiChild()->textCursor().columnNumber() + 1;

        ui->statusBar->showMessage(tr("%1??? %2???").arg(rowNum).arg(colNum), 2000);
    }
}

void MainWindow::initWindow()
{
    setWindowTitle(tr("??????????????????"));

    ui->mainToolBar->setWindowTitle(tr("?????????"));

    ui->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->statusBar->showMessage(tr("??????????????????????????????"));

    QLabel *label = new QLabel(this);
    label->setFrameStyle(QFrame::Box | QFrame::Sunken);
    label->setText(tr("<a href=\"http://www.yafeilinux.com\">yafeilinux.com</a>"));
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    ui->statusBar->addPermanentWidget(label);
    ui->actionNew->setStatusTip(tr("??????????????????"));

    ui->actionOpen->setStatusTip(tr("?????????????????????????????????"));
    ui->actionSave->setStatusTip(tr("?????????????????????"));
    ui->actionSaveAs->setStatusTip(tr("???????????????????????????"));
    ui->actionExit->setStatusTip(tr("??????????????????"));
    ui->actionUndo->setStatusTip(tr("?????????????????????"));
    ui->actionRedo->setStatusTip(tr("?????????????????????"));
    ui->actionCut->setStatusTip(tr("?????????????????????????????????"));
    ui->actionCopy->setStatusTip(tr("?????????????????????????????????"));
    ui->actionPaste->setStatusTip(tr("???????????????????????????????????????"));
    ui->actionClose->setStatusTip(tr("??????????????????"));
    ui->actionCloseAll->setStatusTip(tr("??????????????????"));
    ui->actionTile->setStatusTip(tr("??????????????????"));
    ui->actionCascade->setStatusTip(tr("??????????????????"));
    ui->actionNext->setStatusTip(tr("?????????????????????????????????"));
    ui->actionPrevious->setStatusTip(tr("?????????????????????????????????"));
    ui->actionAbout->setStatusTip(tr("????????????????????????"));
    ui->actionAboutQt->setStatusTip(tr("??????Qt?????????"));
}
