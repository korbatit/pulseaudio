/****************************************************************************
**
** This file is part of pulseaudio plugin for low-level audio backend in Qt4
**
**  pulseaudio Qt4 plugin is free software: you can redistribute it and/or modify
**  it under the terms of the GNU Lesser General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  pulseaudio Qt4 plugin is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public License
**  along with pulseaudio Qt4 plugin.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <QDebug>
#include <QCoreApplication>

#include <QtMultimedia/qaudioformat.h>

#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "pulseaudio.h"

PULSEAudioDeviceInfo::PULSEAudioDeviceInfo(QByteArray dev, QAudio::Mode mode)
{
    device = QLatin1String(dev);
    this->mode = mode;

    updateLists();
}

PULSEAudioDeviceInfo::~PULSEAudioDeviceInfo()
{
    close();
}

bool PULSEAudioDeviceInfo::isFormatSupported(const QAudioFormat& format) const
{
    return testSettings(format);
}

QAudioFormat PULSEAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat nearest;

    nearest.setFrequency(44100);
    nearest.setChannels(2);
    nearest.setByteOrder(QAudioFormat::LittleEndian);
    nearest.setSampleType(QAudioFormat::SignedInt);
    nearest.setSampleSize(16);
    nearest.setCodec(tr("audio/pcm"));

    return nearest;
}

QAudioFormat PULSEAudioDeviceInfo::nearestFormat(const QAudioFormat& format) const
{
    if(testSettings(format))
        return format;
    else
        return preferredFormat();
}

QString PULSEAudioDeviceInfo::deviceName() const
{
    return device;
}

QStringList PULSEAudioDeviceInfo::codecList()
{
    updateLists();
    return codecz;
}

QList<int> PULSEAudioDeviceInfo::frequencyList()
{
    updateLists();
    return freqz;
}

QList<int> PULSEAudioDeviceInfo::channelsList()
{
    updateLists();
    return channelz;
}

QList<int> PULSEAudioDeviceInfo::sampleSizeList()
{
    updateLists();
    return sizez;
}

QList<QAudioFormat::Endian> PULSEAudioDeviceInfo::byteOrderList()
{
    updateLists();
    return byteOrderz;
}

QList<QAudioFormat::SampleType> PULSEAudioDeviceInfo::sampleTypeList()
{
    updateLists();
    return typez;
}

bool PULSEAudioDeviceInfo::open()
{
    // TODO: Need to connect to pulseaudio daemon here?
    return true;
}

void PULSEAudioDeviceInfo::close()
{
    // TODO: Need to disconnect from pulseaudio dameon here?
}

bool PULSEAudioDeviceInfo::testSettings(const QAudioFormat& format) const
{
    if (!channelz.contains(format.channels()))
        return false;
    if (!codecz.contains(format.codec()))
        return false;
    if (!freqz.contains(format.frequency()))
        return false;
    if (!sizez.contains(format.sampleSize()))
        return false;
    if (!byteOrderz.contains(format.byteOrder()))
        return false;
    if (!typez.contains(format.sampleType()))
        return false;

    return true;
}

void PULSEAudioDeviceInfo::updateLists()
{
    freqz.clear();
    channelz.clear();
    sizez.clear();
    byteOrderz.clear();
    typez.clear();
    codecz.clear();

    if(!open())
        return;

    for(int i=0; i<(int)MAX_SAMPLE_RATES; i++) {
        freqz.append(SAMPLE_RATES[i]);
    }
    channelz.append(2);
    sizez.append(16);
    byteOrderz.append(QAudioFormat::LittleEndian);
    byteOrderz.append(QAudioFormat::BigEndian);
    typez.append(QAudioFormat::SignedInt);
    codecz.append(tr("audio/pcm"));

    close();
}

QList<QByteArray> PULSEAudioDeviceInfo::availableDevices(QAudio::Mode mode)
{
    QList<QByteArray> devices;
    if(mode != QAudio::AudioInput)
        devices.append("pulse");
    return devices;
}

PULSEOutputPrivate::PULSEOutputPrivate(PULSEAudioOutput* audio)
{
    audioDevice = audio;
}

PULSEOutputPrivate::~PULSEOutputPrivate() {}

qint64 PULSEOutputPrivate::readData( char* data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}

qint64 PULSEOutputPrivate::writeData(const char* data, qint64 len)
{
    int retry = 0;
    qint64 written = 0;

    if((audioDevice->state() == QAudio::ActiveState)
            ||(audioDevice->state() == QAudio::IdleState)) {
        while(written < len) {
            int chunk = audioDevice->write(data+written,(len-written));
            if(chunk <= 0)
                retry++;
            written+=chunk;
            if(retry > 10)
                return written;
        }
    }
    return written;
}

PULSEAudioOutput::PULSEAudioOutput(const QByteArray &device, const QAudioFormat& format)
{
    bytesAvailable = 0;
    buffer_size = 0;
    period_size = 0;
    buffer_time = 100000;
    period_time = 20000;
    totalTimeValue = 0;
    saveProcessed = 0;
    intervalTime = 1000;
    audioBuffer = 0;
    errorState = QAudio::NoError;
    deviceState = QAudio::StoppedState;
    audioSource = 0;
    pullMode = true;
    handle = 0;

    dummyBuffer = 0;

    settings = format;

    m_device = device;

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),SLOT(userFeed()));
}

PULSEAudioOutput::~PULSEAudioOutput()
{
    close();
    disconnect(timer, SIGNAL(timeout()));
    QCoreApplication::processEvents();
    delete timer;
}

qint64 PULSEAudioOutput::write(const char *data, qint64 len )
{
    qint64 length = len;

    if(!connected)
        return 0;

    writing = true;

    if (dummyBuffer-(int)length < 0)
        length = dummyBuffer;

    if (length == 0) return 0;

    if (pa_simple_write(handle, data, (size_t)length, &err) < 0) {
        qWarning()<<"QAudioOutput::write err, can't write to pulseaudio daemon";
        close();
        connected = false;
        errorState = QAudio::OpenError;
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        return 0;
    } else {
        writeTime.restart();
        totalTimeValue += length;
        dummyBuffer -= (int)length;
        errorState = QAudio::NoError;
        if (deviceState != QAudio::ActiveState) {
            deviceState = QAudio::ActiveState;
            emit stateChanged(deviceState);
        }
        return length;
    }
    return 0;
}

bool PULSEAudioOutput::open()
{
    QTime now(QTime::currentTime());

    clockTime.restart();
    timeStamp.restart();
    writeTime.restart();

    count     = 0;

    params.format = PA_SAMPLE_S16LE;

    if(settings.sampleType() == QAudioFormat::SignedInt) {
        if(settings.sampleSize() == 8) {
            qWarning()<<"unsupported format";
            errorState = QAudio::OpenError;
            deviceState = QAudio::StoppedState;
            emit stateChanged(deviceState);
            return false;

        } else if(settings.sampleSize() == 16) {
            if(settings.byteOrder() == QAudioFormat::LittleEndian)
                params.format = PA_SAMPLE_S16LE;
            else
                params.format = PA_SAMPLE_S16BE;
        }

    } else if(settings.sampleType() == QAudioFormat::UnSignedInt) {
        if(settings.sampleSize() == 8) {
            params.format = PA_SAMPLE_U8;
        } else if(settings.sampleSize() == 16) {
            qWarning()<<"unsupported format";
            errorState = QAudio::OpenError;
            deviceState = QAudio::StoppedState;
            emit stateChanged(deviceState);
            return false;
        }

    } else {
        if(settings.sampleSize() == 8) {
            params.format = PA_SAMPLE_U8;
        } else {
            qWarning()<<"unsupported format";
            errorState = QAudio::OpenError;
            deviceState = QAudio::StoppedState;
            emit stateChanged(deviceState);
            return false;
        }
    }
    params.rate = settings.frequency();
    params.channels = settings.channels();

    memset(&attr,0,sizeof(attr));
    attr.tlength = pa_bytes_per_second(&params)/6;
    attr.maxlength = (attr.tlength*3)/2;
    attr.minreq = attr.tlength/50;
    attr.prebuf = (attr.tlength - attr.minreq)/4;
    attr.fragsize = attr.tlength/50;
    buffer_size = attr.tlength*3;
    period_size = buffer_size/5;

qWarning()<<"f="<<settings.frequency()<<",ch="<<settings.channels()<<", sz="<<settings.sampleSize();

    if(!(handle = pa_simple_new(NULL, m_device.constData(),
                    PA_STREAM_PLAYBACK, NULL,
                    QString("pulseaudio:%1").arg(::getpid()).toAscii().constData(),
                    &params, NULL, &attr, &err))) {
        qWarning()<<"QAudioOutput failed to open, your pulseaudio daemon is not configured correctly";
        errorState = QAudio::OpenError;
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        return false;
    }

    connected = true;
    writing   = false;

    if(audioBuffer == 0)
        audioBuffer = new char[buffer_size];

    if(pullMode)
        connect(audioSource,SIGNAL(readyRead()),this,SLOT(userFeed()));

    timer->start(20);

    errorState  = QAudio::NoError;

    totalTimeValue = 0;
    dummyBuffer = buffer_size;

    return true;
}

void PULSEAudioOutput::userFeed()
{
    if(deviceState == QAudio::StoppedState || deviceState == QAudio::SuspendedState)
        return;

    if(pullMode) {
        // write some audio data and writes it to QIODevice
        while (dummyBuffer >= period_size) {
            int l = audioSource->read(audioBuffer,period_size);
            if(l > 0) {
                qint64 bytesWritten = write(audioBuffer,l);
                if (bytesWritten != l) {
                    audioSource->seek(audioSource->pos()-(l-bytesWritten));
                    break;
                }

            } else if(l == 0) {
                if (deviceState != QAudio::IdleState) {
                    errorState = QAudio::UnderrunError;
                    deviceState = QAudio::IdleState;
                    emit stateChanged(deviceState);
                }
                break;

            } else {
                close();
                errorState = QAudio::IOError;
                emit stateChanged(deviceState);
                break;

            }
        }
    } else {
        if (writeTime.elapsed() > 40 && deviceState != QAudio::IdleState) {
            deviceState = QAudio::IdleState;
            errorState = QAudio::UnderrunError;
            emit stateChanged(deviceState);
        }
    }

    if(intervalTime > 0 && timeStamp.elapsed() > intervalTime) {
        emit notify();
        timeStamp.restart();
    }
    dummyBuffer+=period_size;
    if (dummyBuffer > buffer_size) dummyBuffer = buffer_size;
}

void PULSEAudioOutput::close()
{
    deviceState = QAudio::StoppedState;
    timer->stop();

    if(handle) {
        pa_simple_drain(handle, &err);
        pa_simple_free(handle);
        handle = 0;
        dummyBuffer = buffer_size;
    }
}

QIODevice* PULSEAudioOutput::start(QIODevice* device)
{
    if(deviceState != QAudio::StoppedState)
        deviceState = QAudio::StoppedState;

    errorState = QAudio::NoError;

    // Handle change of mode
    if(audioSource && !pullMode)
        delete audioSource;

    close();

    if (device) {
        audioSource = device;
        pullMode = true;
        deviceState = QAudio::ActiveState;
    } else {
        audioSource = new PULSEOutputPrivate(this);
        audioSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);
        pullMode = false;
        deviceState = QAudio::IdleState;
    }

    if(!open())
        return 0;

    emit stateChanged(deviceState);

    return audioSource;
}

void PULSEAudioOutput::stop()
{
    if(deviceState == QAudio::StoppedState)
        return;
    errorState = QAudio::NoError;
    close();
    emit stateChanged(deviceState);
}

void PULSEAudioOutput::reset()
{
}

void PULSEAudioOutput::suspend()
{
    if(deviceState == QAudio::ActiveState || deviceState == QAudio::IdleState) {
        timer->stop();
        saveProcessed = totalTimeValue;
        close();
        deviceState = QAudio::SuspendedState;
        errorState = QAudio::NoError;
        emit stateChanged(deviceState);
    }
}

void PULSEAudioOutput::resume()
{
    if(deviceState == QAudio::SuspendedState) {
        deviceState = QAudio::ActiveState;
        if(!open()) return;
        totalTimeValue = saveProcessed;
        emit stateChanged(deviceState);
    }
}

int PULSEAudioOutput::bytesFree() const
{
    if(deviceState != QAudio::ActiveState && deviceState != QAudio::IdleState)
        return 0;
    return dummyBuffer;
}

int PULSEAudioOutput::periodSize() const
{
    return period_size;
}

void PULSEAudioOutput::setBufferSize(int value)
{
    buffer_size = value;
}

int PULSEAudioOutput::bufferSize() const
{
    return buffer_size;
}

void PULSEAudioOutput::setNotifyInterval(int ms)
{
    intervalTime = qMax(0, ms);
}

int PULSEAudioOutput::notifyInterval() const
{
    return intervalTime;
}

qint64 PULSEAudioOutput::processedUSecs() const
{
    if (deviceState == QAudio::StoppedState)
        return 0;
    qint64 result = qint64(1000000) * totalTimeValue /
        (settings.channels()*(settings.sampleSize()/8)) /
        settings.frequency();

    return result;
}

qint64 PULSEAudioOutput::elapsedUSecs() const
{
    if(deviceState == QAudio::StoppedState)
        return 0;

    return clockTime.elapsed()*1000;
}

QAudio::Error PULSEAudioOutput::error() const
{
    return errorState;
}

QAudio::State PULSEAudioOutput::state() const
{
    return deviceState;
}

QAudioFormat PULSEAudioOutput::format() const
{
    return settings;
}

void PULSEAudioOutput::setFormat(const QAudioFormat& fmt)
{
    if (deviceState == QAudio::StoppedState)
        settings = fmt;
}
