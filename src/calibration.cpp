#include "calibration.h"
#include "ui_calibration.h"

#include "joycontrolstick.h"
#include "joytabwidget.h"
#include "inputdevice.h"
#include "messagehandler.h"
#include "joybuttontypes/joycontrolstickmodifierbutton.h"

#include <SDL2/SDL_joystick.h>

#include <QTabWidget>
#include <QProgressBar>
#include <QMessageBox>
#include <QLayoutItem>
#include <QPointer>
#include <QDebug>


Calibration::Calibration(QMap<SDL_JoystickID, InputDevice*>* joysticks, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Calibration),
    helper(joysticks->value(0)->getSetJoystick(0)->getJoyStick(0))
{
    ui->setupUi(this);

    qInstallMessageHandler(MessageHandler::myMessageOutput);

    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(trUtf8("Calibration"));

    sumX = 0;
    sumY = 0;
    this->joysticks = joysticks;
    QPointer<JoyControlStick> controlstick = joysticks->value(0)->getSetJoystick(0)->getJoyStick(0);
    this->stick = controlstick.data();
    controlstick.data()->getModifierButton()->establishPropertyUpdatedConnections();
    helper.moveToThread(controlstick.data()->thread());

    setProgressBars(0, 0, 0);
    ui->stickStatusBoxWidget->setFocus();

    ui->stickStatusBoxWidget->setStick(controlstick.data());
    ui->stickStatusBoxWidget->update();

    if (controlstick.isNull())
        controlstick.clear();

    QMapIterator<SDL_JoystickID, InputDevice*> iterTemp(*joysticks);

    while (iterTemp.hasNext())
    {
        iterTemp.next();

        InputDevice *joystick = iterTemp.value();
        QString joytabName = joystick->getSDLName();
        ui->controllersBox->addItem(joytabName);
    }

    for (int i = 0; i < joysticks->value(ui->controllersBox->currentIndex())->getSetJoystick(0)->getNumberSticks(); i++)
    {
       ui->axesBox->addItem(joysticks->value(ui->controllersBox->currentIndex())->getSetJoystick(0)->getJoyStick(i)->getPartialName());
    }

    connect(ui->saveBtn, &QPushButton::clicked, this, &Calibration::saveSettings);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &Calibration::close);
    connect(ui->controllersBox, &QComboBox::currentTextChanged, this, &Calibration::setController);
    connect(ui->axesBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Calibration::createAxesConnection);
    connect(ui->jstestgtkCheckbox, &QCheckBox::stateChanged, this, &Calibration::loadSetFromJstest);
    connect(ui->startButton, &QPushButton::clicked, this, &Calibration::startCalibration);


    update();
}


Calibration::~Calibration()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    delete ui;
}


void Calibration::startCalibration()
{

    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if ((joyAxisX != nullptr) && (joyAxisY != nullptr)) {

            ui->steps->setText(trUtf8("place the joystick in the center position"));
            update();

            this->setWindowTitle(trUtf8("calibrating center"));
            ui->startButton->setText(trUtf8("Start second step"));
            update();

                for (int i = 0; i < x_es_val.count(); i++) {
                  sumX += x_es_val.values().at(i);
                }

                for (int i = 0; i < y_es_val.count(); i++) {
                  sumY += y_es_val.values().at(i);
                }

                if ((sumX != 0) && (sumY != 0)) {
                    joyAxisX->AXIS_CENTER_CALIBRATED = (sumX/x_es_val.count());
                    joyAxisY->AXIS_CENTER_CALIBRATED = (sumY/y_es_val.count());
                } else {
                    joyAxisX->AXIS_CENTER_CALIBRATED = 0;
                    joyAxisY->AXIS_CENTER_CALIBRATED = 0;
                }

                joyAxisX->setDeadZone(joyAxisX->AXIS_CENTER_CALIBRATED + joyAxisX->getDeadZone());
                joyAxisY->setDeadZone(joyAxisY->AXIS_CENTER_CALIBRATED + joyAxisY->getDeadZone());
                QString text = QString();
                text.append(trUtf8("\n\nCenter X: %1").arg(joyAxisX->AXIS_CENTER_CALIBRATED));
                text.append(trUtf8("\nCenter Y: %1").arg(joyAxisY->AXIS_CENTER_CALIBRATED));
                ui->Informations->setText(text);

                x_es_val.clear();
                y_es_val.clear();
                sumX = 0;
                sumY = 0;
                update();

                disconnect(ui->startButton, &QPushButton::clicked, this, nullptr);
                connect(ui->startButton, &QPushButton::clicked, this, &Calibration::startSecondStep);

    }
}


void Calibration::startSecondStep()
{

    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if ((joyAxisX != nullptr) && (joyAxisY != nullptr)) {

            ui->steps->setText(trUtf8("\nplace the joystick in the top-left corner many times"));
            update();

            this->setWindowTitle(trUtf8("calibrating position"));
            update();

            qDebug() << "X_ES_VAL: " << x_es_val.count(QString("-"));
            qDebug() << "Y_ES_VAL: " << y_es_val.count(QString("-"));

            if ((x_es_val.count(QString("-")) > 2) && (y_es_val.count(QString("-")) > 2)) {

                for (int i = 0; i < x_es_val.count(QString("-")); i++) {
                    if ((abs(x_es_val.values(QString("-")).at(i)) > joyAxisX->getDeadZone()))
                        sumX += x_es_val.values(QString("-")).at(i);
                }

                for (int i = 0; i < y_es_val.count(QString("-")); i++) {
                    if ((abs(y_es_val.values(QString("-")).at(i)) > joyAxisY->getDeadZone()))
                        sumY += y_es_val.values(QString("-")).at(i);
                }

                joyAxisX->AXIS_MIN_CALIBRATED = sumX / x_es_val.count(QString("-"));
                joyAxisY->AXIS_MIN_CALIBRATED = sumY / y_es_val.count(QString("-"));

                QString text = ui->Informations->text();
                text.append(trUtf8("\n\nX: %1").arg(joyAxisX->AXIS_MIN_CALIBRATED));
                text.append(trUtf8("\nY: %1").arg(joyAxisY->AXIS_MIN_CALIBRATED));
                ui->Informations->setText(text);
                update();

                x_es_val.clear();
                y_es_val.clear();
                sumX = 0;
                sumY = 0;

                disconnect(ui->startButton, &QPushButton::clicked, this, nullptr);
                connect(ui->startButton, &QPushButton::clicked, this, &Calibration::startLastStep);
        }
    }
}


void Calibration::startLastStep()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if ((joyAxisX != nullptr) && (joyAxisY != nullptr)) {

            ui->steps->setText(trUtf8("\nplace the joystick in the bottom-right corner"));
            update();

            this->setWindowTitle(trUtf8("calibrating position"));
            ui->startButton->setText(trUtf8("Start final step"));
            update();

            if ((x_es_val.count(QString("+")) > 2) && (y_es_val.count(QString("+")) > 2)) {

                for (int i = 0; i < x_es_val.count(QString("+")); i++) {
                    if ((x_es_val.values(QString("+")).at(i) > joyAxisX->getDeadZone()))
                        sumX += x_es_val.values(QString("+")).at(i);
                }

                for (int i = 0; i < y_es_val.count(QString("+")); i++) {
                    if ((y_es_val.values(QString("+")).at(i) > joyAxisY->getDeadZone()))
                        sumY += y_es_val.values(QString("+")).at(i);
                }

                joyAxisX->AXIS_MAX_CALIBRATED = sumX / x_es_val.count(QString("+"));
                joyAxisY->AXIS_MAX_CALIBRATED = sumY / y_es_val.count(QString("+"));

                QString text2 = ui->Informations->text();
                text2.append(trUtf8("\n\nX: %1").arg(joyAxisX->AXIS_MAX_CALIBRATED));
                text2.append(trUtf8("\nY: %1").arg(joyAxisY->AXIS_MAX_CALIBRATED));
                ui->Informations->setText(text2);
                update();

                if(joyAxisX->AXIS_MAX_CALIBRATED > abs(joyAxisX->AXIS_MIN_CALIBRATED)) {
                    joyAxisX->AXIS_MAX_CALIBRATED = abs(joyAxisX->AXIS_MIN_CALIBRATED);
                } else {
                    joyAxisX->AXIS_MIN_CALIBRATED =  -(joyAxisX->AXIS_MAX_CALIBRATED);
                }

                if(joyAxisY->AXIS_MAX_CALIBRATED > abs(joyAxisY->AXIS_MIN_CALIBRATED)) {
                    joyAxisY->AXIS_MAX_CALIBRATED = abs(joyAxisY->AXIS_MIN_CALIBRATED);
                } else {
                    joyAxisY->AXIS_MIN_CALIBRATED =  -(joyAxisY->AXIS_MAX_CALIBRATED);
                }

                QString text3 = ui->Informations->text();
                text3.append(trUtf8("\n\nrange X: %1 - %2").arg(joyAxisX->AXIS_MIN_CALIBRATED).arg(joyAxisX->AXIS_MAX_CALIBRATED));
                text3.append(trUtf8("\nrange Y: %1 - %2").arg(joyAxisY->AXIS_MIN_CALIBRATED).arg(joyAxisY->AXIS_MAX_CALIBRATED));
                ui->Informations->setText(text3);
                update();

                ui->steps->setText(trUtf8("\n---calibration done---\n"));
                update();

                ui->startButton->setText(trUtf8("Start calibration"));
                disconnect(ui->startButton, &QPushButton::clicked, this, nullptr);
                connect(ui->startButton, &QPushButton::clicked, this, &Calibration::startCalibration);

            }
    }
}


void Calibration::saveSettings()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (enoughProb()) {

        if (!x_es_val.isEmpty() && !y_es_val.isEmpty() && (joyAxisX != nullptr) && (joyAxisY != nullptr)) {

            int rawValX = calculateRawVal(x_es_val, joyAxisX);
            int rawValY = calculateRawVal(y_es_val, joyAxisY);
            qDebug() << "rawValx: " << rawValX;
            qDebug() << "rawValy: " << rawValY;
           // stick->getAxisX()->setInitialThrottle(stick->getAxisX()->calculateThrottledValue());
            //int JoyAxis::calculateThrottledValue(int value)
            calibrate(stick);

            setInfoText(stick->calculateXDiagonalDeadZone(joyAxisX->getCurrentRawValue(), joyAxisY->getCurrentRawValue()), stick->calculateYDiagonalDeadZone(joyAxisX->getCurrentRawValue(), joyAxisY->getCurrentRawValue()));
            ui->stickStatusBoxWidget->update();
            update();
            QMessageBox::information(this, trUtf8("Save"), trUtf8("Dead zone values have been saved"));
        }

    } else {



    }
}


const QString Calibration::getSetfromGtkJstest()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return QString();
}


void Calibration::setInfoText(int deadZoneX, int deadZoneY)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->Informations->setText(trUtf8("dead zone value - X: %1\ndead zone value - Y: %2\n").arg(deadZoneX).arg(deadZoneY));
}


bool Calibration::enoughProb()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    bool enough = true;

    if (x_es_val.count(QString("+")) < 5) { enough = false; QMessageBox::information(this, trUtf8("Dead zone calibration"), trUtf8("You must move X axis to the right at least three times!")); }
    else if (x_es_val.count(QString("-")) < 5) { enough = false; QMessageBox::information(this, trUtf8("Dead zone calibration"), trUtf8("You must move X axis to the left at least three times!")); }
    else if (y_es_val.count(QString("+")) < 5) { enough = false; QMessageBox::information(this, trUtf8("Dead zone calibration"), trUtf8("You must move Y axis up at least three times!")); }
    else if (y_es_val.count(QString("-")) < 5) { enough = false; QMessageBox::information(this, trUtf8("Dead zone calibration"), trUtf8("You must move Y axis down at least three times!")); }

    return enough;
}


int Calibration::calculateRawVal(QHash<QString,int> ax_values, JoyAxis* joyAxis)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int min_int = chooseMinMax(QString("-"), ax_values.values(QString("-")));
    int max_int = chooseMinMax(QString("+"), ax_values.values(QString("+")));
    int deadzone_value = 0;

    qDebug() << "Int Min value of " << joyAxis->getAxisName() << " is " << min_int;
    qDebug() << "Int Max value of " << joyAxis->getAxisName() << " is " << max_int;

    if (abs(min_int) == max_int) {

        qDebug() << "Min and max values are equal";

        deadzone_value = max_int;

    } else {

        if (max_int > abs(min_int)) {

            qDebug() << "Max value is greater than min value";

            deadzone_value = min_int;

        } else {

            qDebug() << "Min value is greater than max value";

            deadzone_value = max_int;

        }
    }

    joyAxis->setCurrentRawValue(deadzone_value);
    int axis1Value = joyAxis->getCurrentRawValue();

    qDebug() << "current raw value for joyAxis is " << axis1Value;

    return axis1Value;
}


void Calibration::calibrate(JoyControlStick* stick)
{
   /* if (stick->inDeadZone()) {

        // stickInput.normalized * ((stickInput.magnitude - deadzone) / (1 - deadzone));

        double norm = stick->getNormalizedAbsoluteDistance() * ((stick->getAbsoluteRawDistance(stick->getAxisX()->getCurrentRawValue(), stick->getAxisY()->getCurrentRawValue()) - stick->getDeadZone()) / (1 - stick->getDeadZone()));

        qDebug() << "Stick values are in dead zone circle: " << norm;

        stick->getAxisX()->setCurrentRawValue(norm);
        stick->getAxisY()->setCurrentRawValue(norm);

    } else {

        qDebug() << "Stick doesn't appear in dead zone circle. 0 values are set into joyAxisX and joyAxisY";

        stick->getAxisX()->setCurrentRawValue(0);
        stick->getAxisY()->setCurrentRawValue(0);

    }*/
}


int Calibration::chooseMinMax(QString min_max_sign, QList<int> ax_values)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int min_max = 0;

    foreach(int val, ax_values) {

        if (min_max_sign == QString("+")) {
            if (min_max < val) min_max = val;

        } else {
            if (min_max > val) min_max = val;
        }
    }

    return min_max;
}


void Calibration::checkX(int value)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (value > 0) {
        if (x_es_val.count(QString("+")) <= 100) x_es_val.insert(QString("+"), value);
    } else if (value < 0) {
        if (x_es_val.count(QString("-")) <= 100) x_es_val.insert(QString("-"), value);
    }

    axisBarX->setValue(value);
    update();
}


void Calibration::checkY(int value)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (value > 0) {
        if (y_es_val.count(QString("+")) <= 100) y_es_val.insert(QString("+"), value);
    } else if (value < 0) {
        if (y_es_val.count(QString("-")) <= 100) y_es_val.insert(QString("-"), value);
    }

    axisBarY->setValue(value);
    update();
}


void Calibration::setController(QString controllerName)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QMapIterator<SDL_JoystickID, InputDevice*> iterTemp(*joysticks);

    while (iterTemp.hasNext())
    {
        iterTemp.next();

        QPointer<InputDevice> joystick = iterTemp.value();

        if (controllerName == joystick.data()->getSDLName())
        {
            updateAxesBox();
            createAxesConnection();
            break;
        }

        if (joystick.isNull()) joystick.clear();
    }
}


void Calibration::updateAxesBox()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->axesBox->clear();

    for (int i = 0; i < joysticks->value(ui->controllersBox->currentIndex())->getSetJoystick(0)->getNumberSticks(); i++)
    {
       ui->axesBox->addItem(joysticks->value(ui->controllersBox->currentIndex())->getSetJoystick(0)->getJoyStick(i)->getPartialName());
    }

    update();
}


void Calibration::loadSetFromJstest()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

}


bool Calibration::ifGtkJstestRunToday()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return true;
}


void Calibration::createAxesConnection()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    qDeleteAll(ui->axesWidget->findChildren<QWidget*>());

    QPointer<JoyControlStick> controlstick = joysticks->value(ui->controllersBox->currentIndex())->getSetJoystick(0)->getJoyStick(ui->axesBox->currentIndex());
    this->stick = controlstick.data();
    controlstick.data()->getModifierButton()->establishPropertyUpdatedConnections();
    helper.moveToThread(controlstick.data()->thread());
    ui->stickStatusBoxWidget->setStick(controlstick.data());
    ui->stickStatusBoxWidget->update();
    setProgressBars(controlstick.data());

    if (controlstick.isNull()) controlstick.clear();
}


void Calibration::setProgressBars(JoyControlStick* controlstick)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

        joyAxisX = controlstick->getAxisX();
        joyAxisY = controlstick->getAxisY();

        if ((joyAxisX != nullptr) && (joyAxisY != nullptr))
        {
            QHBoxLayout *hbox = new QHBoxLayout();
            QHBoxLayout *hbox2 = new QHBoxLayout();

            QLabel *axisLabel = new QLabel();
            QLabel *axisLabel2 = new QLabel();
            axisLabel->setText(trUtf8("Axis %1").arg(joyAxisX->getRealJoyIndex()));
            axisLabel2->setText(trUtf8("Axis %1").arg(joyAxisY->getRealJoyIndex()));
            axisBarX = new QProgressBar();
            axisBarY = new QProgressBar();
            axisBarX->setMinimum(JoyAxis::AXISMIN);
            axisBarX->setMaximum(JoyAxis::AXISMAX);
            axisBarX->setFormat("%v");
            axisBarX->setValue(joyAxisX->getCurrentRawValue());
            axisBarY->setMinimum(JoyAxis::AXISMIN);
            axisBarY->setMaximum(JoyAxis::AXISMAX);
            axisBarY->setFormat("%v");
            axisBarY->setValue(joyAxisY->getCurrentRawValue());
            hbox->addWidget(axisLabel);
            hbox->addWidget(axisBarX);
            hbox->addSpacing(10);
            hbox2->addWidget(axisLabel2);
            hbox2->addWidget(axisBarY);
            hbox2->addSpacing(10);
            ui->progressBarsLayout->addLayout(hbox);
            ui->progressBarsLayout->addLayout(hbox2);

            connect(joyAxisX, &JoyAxis::moved, this, &Calibration::checkX);
            connect(joyAxisY, &JoyAxis::moved, this, &Calibration::checkY);

        }

        update();
}


void Calibration::setProgressBars(int inputDevNr, int setJoyNr, int stickNr)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

        JoyControlStick* controlstick = joysticks->value(inputDevNr)->getSetJoystick(setJoyNr)->getJoyStick(stickNr);
        helper.moveToThread(controlstick->thread());

        joyAxisX = controlstick->getAxisX();
        joyAxisY = controlstick->getAxisY();

        if ((joyAxisX != nullptr) && (joyAxisY != nullptr))
        {
            QHBoxLayout *hbox = new QHBoxLayout();
            QHBoxLayout *hbox2 = new QHBoxLayout();

            QLabel *axisLabel = new QLabel();
            QLabel *axisLabel2 = new QLabel();
            axisLabel->setText(trUtf8("Axis %1").arg(joyAxisX->getRealJoyIndex()));
            axisLabel2->setText(trUtf8("Axis %1").arg(joyAxisY->getRealJoyIndex()));
            axisBarX = new QProgressBar();
            axisBarY = new QProgressBar();
            axisBarX->setMinimum(JoyAxis::AXISMIN);
            axisBarX->setMaximum(JoyAxis::AXISMAX);
            axisBarX->setFormat("%v");
            axisBarX->setValue(joyAxisX->getCurrentRawValue());
            axisBarY->setMinimum(JoyAxis::AXISMIN);
            axisBarY->setMaximum(JoyAxis::AXISMAX);
            axisBarY->setFormat("%v");
            axisBarY->setValue(joyAxisY->getCurrentRawValue());
            hbox->addWidget(axisLabel);
            hbox->addWidget(axisBarX);
            hbox->addSpacing(10);
            hbox2->addWidget(axisLabel2);
            hbox2->addWidget(axisBarY);
            hbox2->addSpacing(10);
            ui->progressBarsLayout->addLayout(hbox);
            ui->progressBarsLayout->addLayout(hbox2);

            connect(joyAxisX, &JoyAxis::moved, this, &Calibration::checkX);
            connect(joyAxisY, &JoyAxis::moved, this, &Calibration::checkY);

        }

        update();
}
