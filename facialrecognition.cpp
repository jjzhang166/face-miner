#include "facialrecognition.h"
#include "ui_facialrecognition.h"

FacialRecognition::FacialRecognition(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FacialRecognition)
{
    ui->setupUi(this);

    cv::VideoCapture _cam(0);
    if (!_cam.isOpened()) {
        QMessageBox error(this);
        error.critical(this, "Error", "Unable to open webcam");
        error.show();
    }


    // register cv::Mat type, thus it can be used in signals and slots
    qRegisterMetaType<cv::Mat>("cv::Mat");

    QThread *frameStreamThread = new QThread();
    CamStream *frameStream = new CamStream(_cam);
    // Move the object frameStream to his own thread
    frameStream->moveToThread(frameStreamThread);

    // sending CamStream::newFrame output, into FacialRecognition::updateCamView input
    connect(frameStream, &CamStream::newFrame, this, &FacialRecognition::_updateCamView);

    // start frameStream on frameStreamThread start
    connect(frameStreamThread, &QThread::started, frameStream, &CamStream::start);

    // sending click coordinates into CamStremView to FacialRecognition::_handleClick
    connect(_getCamStreamView(),&VideoStreamView::clicked, this, &FacialRecognition::_handleClick);

    // set CamStreamView to fixed size
    QSize streamSize(300,300);
    _getCamStreamView()->setSize(streamSize);

    // set TraingStreamView to fixed size
    _getTrainingStreamView()->setSize(streamSize);
    // start thread: stream of frames
    frameStreamThread->start();

    // Create a thread for the miner
    QThread *minerThread = new QThread();
    // Create the face pattern miner
    // TODO: user BioID dataset for validation. Use yale dataset for training.
    FacePatternMiner *patternMiner = new FacePatternMiner("./datasets/BioID-FaceDatabase-V1.2/");
    // move the miner to his own thread
    patternMiner->moveToThread(minerThread);
    // connect signal start of the thread to the start() method of the miner
    connect(minerThread, &QThread::started, patternMiner, &FacePatternMiner::start);
    // connect processing signal of miner to the GUI, to show what image has being processed
    connect(patternMiner, &FacePatternMiner::preprocessing, this, &FacialRecognition::_updateTrainingStreamView);
    // start the miner thread
    minerThread->start();

}

void FacialRecognition::_updateCamView(const cv::Mat& frame)
{
    _getCamStreamView()->setImage(Cv2Qt::cvMatToQImage(frame));
}

void FacialRecognition::_updateTrainingStreamView(const cv::Mat& frame) {
    _getTrainingStreamView()->setImage(Cv2Qt::cvMatToQImage(frame));
}

void FacialRecognition::_handleClick(const cv::Point& point)
{
    std::cout << point << std::endl;
}

FacialRecognition::~FacialRecognition()
{
    delete ui;
}

VideoStreamView* FacialRecognition::_getCamStreamView() {
    return reinterpret_cast<VideoStreamView*>(ui->gridLayout->itemAtPosition(0,0)->widget());
}

VideoStreamView* FacialRecognition::_getTrainingStreamView() {
    return reinterpret_cast<VideoStreamView*>(ui->gridLayout->itemAtPosition(0,2)->widget());
}
