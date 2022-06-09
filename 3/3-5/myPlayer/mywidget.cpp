#include "mywidget.h"
#include "ui_mywidget.h"
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QTime>
#include <QMessageBox>
#include <QFileInfo>

#include "myplaylist.h"
#include <QFileDialog>
#include <QDesktopServices>

#include "mylrc.h"
#include <QTextCodec>

#include <QMenu>
#include <QCloseEvent>

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyWidget)
{
    ui->setupUi(this);
    initPlayer();
}

MyWidget::~MyWidget()
{
    delete ui;
}

void MyWidget::initPlayer()
{
    setWindowTitle(tr("MyPlayer音乐播放器"));
    setWindowIcon(QIcon(":/images/icon.png"));
    setMinimumSize(320, 160);
    setMaximumSize(320, 160);

    mediaObject = new Phonon::MediaObject(this);
    Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    Phonon::createPath(mediaObject, audioOutput);

    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(updateTime(qint64)));

    topLabel = new QLabel(tr("<a href = \" http://www.yafeilinux.com \"> www.yafeilinux.com </a>"));
    topLabel->setTextFormat(Qt::RichText);
    topLabel->setOpenExternalLinks(true);
    topLabel->setAlignment(Qt::AlignCenter);

    Phonon::SeekSlider *seekSlider = new Phonon::SeekSlider(mediaObject, this);

    QToolBar *widgetBar = new QToolBar(this);

    timeLabel = new QLabel(tr("00:00 / 00:00"), this);
    timeLabel->setToolTip(tr("当前时间 / 总时间"));
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QAction *PLAction = new QAction(tr("PL"), this);
    PLAction->setShortcut(QKeySequence("F4"));
    PLAction->setToolTip(tr("播放列表(F4)"));
    connect(PLAction, SIGNAL(triggered()), this, SLOT(setPlaylistShown()));

    QAction *LRCAction = new QAction(tr("LRC"), this);
    LRCAction->setShortcut(QKeySequence("F2"));
    LRCAction->setToolTip(tr("桌面歌词(F2)"));
    connect(LRCAction, SIGNAL(triggered()), this, SLOT(setLrcShown()));

    widgetBar->addAction(PLAction);
    widgetBar->addSeparator();
    widgetBar->addWidget(timeLabel);
    widgetBar->addSeparator();
    widgetBar->addAction(LRCAction);

    QToolBar *toolBar = new QToolBar(this);

    playAction = new QAction(this);
    playAction->setIcon(QIcon(":/images/play.png"));
    playAction->setText(tr("播放(F5)"));
    playAction->setShortcut(QKeySequence(tr("F5")));
    connect(playAction, SIGNAL(triggered()), this, SLOT(setPaused()));

    stopAction = new QAction(this);
    stopAction->setIcon(QIcon(":/images/stop.png"));
    stopAction->setText(tr("停止(F6)"));
    stopAction->setShortcut(QKeySequence(tr("F6")));
    connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));

    skipBackwardAction = new QAction(this);
    skipBackwardAction->setIcon(QIcon(":/images/skipBackward.png"));
    skipBackwardAction->setText(tr("上一首(Ctrl+Left)"));
    skipBackwardAction->setShortcut(QKeySequence(tr("Ctrl+Left")));
    connect(skipBackwardAction, SIGNAL(triggered()), this, SLOT(skipBackward()));

    skipForwardAction = new QAction(this);
    skipForwardAction->setIcon(QIcon(":/images/skipForward.png"));
    skipForwardAction->setText(tr("下一首(Ctrl+Right)"));
    skipForwardAction->setShortcut(QKeySequence(tr("Ctrl+Right")));
    connect(skipForwardAction, SIGNAL(triggered()), this, SLOT(skipForward()));

    QAction *openAction = new QAction(this);
    openAction->setIcon(QIcon(":/images/open.png"));
    openAction->setText(tr("播放文件(Ctrl+O)"));
    openAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    Phonon::VolumeSlider *volumeSlider = new Phonon::VolumeSlider(audioOutput, this);
    volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    toolBar->addAction(playAction);
    toolBar->addSeparator();
    toolBar->addAction(stopAction);
    toolBar->addSeparator();
    toolBar->addAction(skipBackwardAction);
    toolBar->addSeparator();
    toolBar->addAction(skipForwardAction);
    toolBar->addSeparator();
    toolBar->addWidget(volumeSlider);
    toolBar->addSeparator();
    toolBar->addAction(openAction);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(topLabel);
    mainLayout->addWidget(seekSlider);
    mainLayout->addWidget(widgetBar);
    mainLayout->addWidget(toolBar);
    setLayout(mainLayout);

    connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SLOT(stateChanged(Phonon::State,Phonon::State)));

    playlist = new MyPlaylist(this);
    connect(playlist, SIGNAL(cellClicked(int,int)), this, SLOT(tableClicked(int)));
    connect(playlist, SIGNAL(playlistClean()), this, SLOT(clearSources()));

    metaInformationResolver = new Phonon::MediaObject(this);

    Phonon::AudioOutput *metaInformationAudioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    Phonon::createPath(metaInformationResolver, metaInformationAudioOutput);
    connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State, Phonon::State)), this, SLOT(metaStateChanged(Phonon::State, Phonon::State)));

    connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)), this, SLOT(sourceChanged(Phonon::MediaSource)));
    connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

    playAction->setEnabled(false);
    stopAction->setEnabled(false);
    skipBackwardAction->setEnabled(false);
    skipForwardAction->setEnabled(false);
    topLabel->setFocus();

    lrc = new MyLrc(this);

    trayIcon = new QSystemTrayIcon(QIcon(":/images/icon.png"), this);
    trayIcon->setToolTip(tr("MyPlayer音乐播放器 --- yafeilinux作品"));

    QMenu *menu = new QMenu;
    QList<QAction *> actions;
    actions << playAction << stopAction << skipBackwardAction << skipForwardAction;
    menu->addActions(actions);
    menu->addSeparator();
    menu->addAction(PLAction);
    menu->addAction(LRCAction);
    menu->addSeparator();
    menu->addAction(tr("退出"), qApp, SLOT(quit()));
    trayIcon->setContextMenu(menu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();
}

void MyWidget::updateTime(qint64 time)
{
    qint64 totalTimeValue = mediaObject->totalTime();
    QTime totalTime(0, (totalTimeValue / 60000) % 60, (totalTimeValue / 1000) % 60);
    QTime currentTime(0, (time / 60000) % 60, (time / 1000) % 60);
    QString str = currentTime.toString("mm:ss") + " / " + totalTime.toString("mm:ss");
    timeLabel->setText(str);

    if (!lrcMap.isEmpty())
    {
        qint64 previous = 0;
        qint64 later = 0;
        foreach (qint64 value, lrcMap.keys())
        {
            if (time >= value)
            {
                previous = value;
            }
            else
            {
                later = value;
                break;
            }
        }

        if (later == 0)
            later = totalTimeValue;

        QString currentLrc = lrcMap.value(previous);

        if (currentLrc.length() < 2)
            currentLrc = tr("MyPlayer音乐播放器 --- yafeilinux作品");

        if (currentLrc != lrc->text())
        {
            lrc->setText(currentLrc);
            topLabel->setText(currentLrc);
            qint64 intervalTime = later - previous;
            lrc->startLrcMask(intervalTime);
        }
    }
    else
    {
        topLabel->setText(QFileInfo(mediaObject->currentSource().fileName()).baseName());
    }
}

void MyWidget::setPaused()
{
    if (mediaObject->state() == Phonon::PlayingState)
        mediaObject->pause();
    else
        mediaObject->play();
}

void MyWidget::skipBackward()
{
    lrc->stopLrcMask();
    int index = sources.indexOf(mediaObject->currentSource());
    mediaObject->setCurrentSource(sources.at(index - 1));
    mediaObject->play();
}

void MyWidget::skipForward()
{
    lrc->stopLrcMask();
    int index = sources.indexOf(mediaObject->currentSource());
    mediaObject->setCurrentSource(sources.at(index + 1));
    mediaObject->play();
}

void MyWidget::openFile()
{
    QStringList list = QFileDialog::getOpenFileNames(this, tr("打开音乐文件"), QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    if (list.isEmpty())
        return;

    int index = sources.size();

    foreach (QString string, list)
    {
        Phonon::MediaSource source(string);
        sources.append(source);
    }

    if (!sources.isEmpty())
        metaInformationResolver->setCurrentSource(sources.at(index));
}

void MyWidget::setPlaylistShown()
{
    if (playlist->isHidden())
    {
        playlist->move(frameGeometry().bottomLeft());
        playlist->show();
    }
    else
    {
        playlist->hide();
    }
}

void MyWidget::setLrcShown()
{
    if (lrc->isHidden())
        lrc->show();
    else
        lrc->hide();
}

void MyWidget::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    switch (newState)
    {
    case Phonon::ErrorState:
        if (mediaObject->errorType() == Phonon::FatalError)
        {
            QMessageBox::warning(this, tr("致命错误"), mediaObject->errorString());
        }
        else
        {
            QMessageBox::warning(this, tr("错误"), mediaObject->errorString());
        }
        break;
    case Phonon::PlayingState:
        stopAction->setEnabled(true);
        playAction->setIcon(QIcon(":/images/pause.png"));
        playAction->setText(tr("暂停(F5)"));
        topLabel->setText(QFileInfo(mediaObject->currentSource().fileName()).baseName());
        resolveLrc(mediaObject->currentSource().fileName());
        break;
    case Phonon::StoppedState:
        stopAction->setEnabled(false);
        playAction->setIcon(QIcon(":/images/play.png"));
        playAction->setText(tr("播放(F5)"));
        topLabel->setText(tr("<a href = \" http://www.yafeilinux.com \">www.yafeilinux.com</a>"));
        timeLabel->setText(tr("00:00 / 00:00"));
        lrc->stopLrcMask();
        lrc->setText(tr("MyPlayer音乐播放器 --- yafeilinux作品"));
        break;
    case Phonon::PausedState:
        stopAction->setEnabled(true);
        playAction->setIcon(QIcon(":/images/play.png"));
        playAction->setText(tr("播放(F5)"));
        topLabel->setText(QFileInfo(mediaObject->currentSource().fileName()).baseName() + tr(" 已暂停！"));
        if (!lrcMap.isEmpty())
        {
            lrc->stopLrcMask();
            lrc->setText(topLabel->text());
        }
        break;
    case Phonon::BufferingState:
        break;
    default:
        ;
    }
}

void MyWidget::metaStateChanged(Phonon::State newState, Phonon::State oldState)
{
    if (newState == Phonon::ErrorState)
    {
        QMessageBox::warning(this, tr("打开文件时出错"), metaInformationResolver->errorString());
        while (!sources.isEmpty() && !(sources.takeLast() == metaInformationResolver->currentSource()))
        {};
        return;
    }

    if (newState != Phonon::StoppedState && newState != Phonon::PausedState)
        return;

    if (metaInformationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
        return;

    QMap<QString, QString> metaData = metaInformationResolver->metaData();

    QString title = metaData.value("TITLE");
    if (title == "")
    {
        QString str = metaInformationResolver->currentSource().fileName();
        title = QFileInfo(str).baseName();
    }
    QTableWidgetItem *titleItem = new QTableWidgetItem(title);
    titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);

    QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
    artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);

    qint64 totalTime = metaInformationResolver->totalTime();
    QTime time(0, (totalTime / 60000) % 60, (totalTime / 1000) % 60);
    QTableWidgetItem *timeItem = new QTableWidgetItem(time.toString("mm:ss"));

    int currentRow = playlist->rowCount();
    playlist->insertRow(currentRow);
    playlist->setItem(currentRow, 0, titleItem);
    playlist->setItem(currentRow, 1, artistItem);
    playlist->setItem(currentRow, 2, timeItem);

    int index = sources.indexOf(metaInformationResolver->currentSource()) + 1;
    if (sources.size() > index)
    {
        metaInformationResolver->setCurrentSource(sources.at(index));
    }
    else
    {
        if (playlist->selectedItems().isEmpty())
        {
            if (mediaObject->state() != Phonon::PlayingState && mediaObject->state() != Phonon::PausedState)
            {
                mediaObject->setCurrentSource(sources.at(0));
            }
            else
            {
                playlist->selectRow(0);
                changeActionState();
            }
        }
        else
        {
            changeActionState();
        }
    }
}

void MyWidget::changeActionState()
{
    if (sources.count() == 0)
    {
        if (mediaObject->state() != Phonon::PlayingState && mediaObject->state() != Phonon::PausedState)
        {
            playAction->setEnabled(false);
            stopAction->setEnabled(false);
        }
        skipBackwardAction->setEnabled(false);
        skipForwardAction->setEnabled(false);
    }
    else
    {
        playAction->setEnabled(true);
        stopAction->setEnabled(true);
        if (sources.count() == 1)
        {
            skipBackwardAction->setEnabled(false);
            skipForwardAction->setEnabled(false);
        }
        else
        {
            skipBackwardAction->setEnabled(true);
            skipForwardAction->setEnabled(true);
            int index = playlist->currentRow();
            if (index == 0)
                skipBackwardAction->setEnabled(false);
            if (index + 1 == sources.count())
                skipForwardAction->setEnabled(false);
        }
    }
}

void MyWidget::sourceChanged(const Phonon::MediaSource &source)
{
    int index = sources.indexOf(source);
    playlist->selectRow(index);
    changeActionState();
}

void MyWidget::aboutToFinish()
{
    int index = sources.indexOf(mediaObject->currentSource()) + 1;
    if (sources.size() > index)
    {
        mediaObject->enqueue(sources.at(index));
        mediaObject->seek(mediaObject->totalTime());
        lrc->stopLrcMask();
        resolveLrc(sources.at(index).fileName());
    }
    else
    {
        mediaObject->stop();
    }
}

void MyWidget::tableClicked(int row)
{
    bool wasPlaying = mediaObject->state() == Phonon::PlayingState;
    mediaObject->stop();
    mediaObject->clearQueue();

    if (row >= sources.size())
        return;

    mediaObject->setCurrentSource(sources.at(row));

    if (wasPlaying)
        mediaObject->play();
}

void MyWidget::clearSources()
{
    sources.clear();

    changeActionState();
}

void MyWidget::resolveLrc(const QString &sourceFileName)
{
    lrcMap.clear();

    if (sourceFileName.isEmpty())
        return;
    QString fileName = sourceFileName;
    QString lrcFileName = fileName.remove(fileName.right(3)) + "lrc";

    QFile file(lrcFileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        lrc->setText(QFileInfo(mediaObject->currentSource().fileName()).baseName() + tr(" --- 未找到歌词文件！"));
        return;
    }
    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());
    QString allText = QString(file.readAll());
    file.close();
    QStringList lines = allText.split("\n");

    QRegExp rx("\\[\\d{2}:\\d{2}\\.\\d{2}\\]");
    foreach (QString oneLine, lines)
    {
        QString temp = oneLine;
        temp.replace(rx, "");
        int pos = rx.indexIn(oneLine, 0);
        while (pos != -1)
        {
            QString cap = rx.cap(0);

            QRegExp regexp;
            regexp.setPattern("\\d{2}(?=:)");
            regexp.indexIn(cap);
            int minute = regexp.cap(0).toInt();
            regexp.setPattern("\\d{2}(?=\\.)");
            regexp.indexIn(cap);
            int second = regexp.cap(0).toInt();
            regexp.setPattern("\\d{2}(?=\\])");
            regexp.indexIn(cap);
            int millisecond = regexp.cap(0).toInt();
            qint64 totalTime = minute * 60000 + second * 1000 + millisecond * 10;

            lrcMap.insert(totalTime, temp);
            pos += rx.matchedLength();
            pos = rx.indexIn(oneLine, pos);
        }
    }
    if (lrcMap.isEmpty())
    {
        lrc->setText(QFileInfo(mediaObject->currentSource().fileName()).baseName() + tr(" --- 歌词文件内容错误！"));
        return;
    }
}

void MyWidget::trayIconActivated(QSystemTrayIcon::ActivationReason activationReason)
{
    if (activationReason == QSystemTrayIcon::Trigger)
    {
        show();
    }
}

void MyWidget::closeEvent(QCloseEvent *event)
{
    if (isVisible())
    {
        hide();
        trayIcon->showMessage(tr("MyPlayer音乐播放器"), tr("点我重新显示主界面"));
        event->ignore();
    }
}
