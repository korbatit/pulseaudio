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

#ifndef QPULSEAUDIO_H
#define QPULSEAUDIO_H

#include <QObject>
#include <QTime>
#include <QTimer>
#include <QByteArray>
#include <QIODevice>

#include <QtMultimedia>

#include <pulse/simple.h>
#include <pulse/error.h>

const unsigned int MAX_SAMPLE_RATES = 5;
const unsigned int SAMPLE_RATES[] =
    { 8000, 11025, 22050, 44100, 48000 };

class PULSEAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT
public:
    PULSEAudioDeviceInfo(QByteArray dev, QAudio::Mode mode);
    ~PULSEAudioDeviceInfo();

    bool testSettings(const QAudioFormat& format) const;
    void updateLists();
    QAudioFormat preferredFormat() const;
    bool isFormatSupported(const QAudioFormat& format) const;
    QAudioFormat nearestFormat(const QAudioFormat& format) const;
    QString deviceName() const;
    QStringList codecList();
    QList<int> frequencyList();
    QList<int> channelsList();
    QList<int> sampleSizeList();
    QList<QAudioFormat::Endian> byteOrderList();
    QList<QAudioFormat::SampleType> sampleTypeList();
    QList<QByteArray> availableDevices(QAudio::Mode);

private:
    bool open();
    void close();

    QString device;
    QAudio::Mode mode;
    QAudioFormat settings;
    QAudioFormat nearest;
    QList<int> freqz;
    QList<int> channelz;
    QList<int> sizez;
    QList<QAudioFormat::Endian> byteOrderz;
    QStringList codecz;
    QList<QAudioFormat::SampleType> typez;
    int fd;
};

class PULSEAudioOutput;

class PULSEOutputPrivate : public QIODevice
{
    Q_OBJECT
public:
    PULSEOutputPrivate(PULSEAudioOutput* audio);
    ~PULSEOutputPrivate();

    qint64 readData( char* data, qint64 len);
    qint64 writeData(const char* data, qint64 len);

private:
    PULSEAudioOutput *audioDevice;
};

class PULSEAudioOutput : public QAbstractAudioOutput
{
    friend class PULSEOutputPrivate;
    Q_OBJECT
public:
    PULSEAudioOutput(const QByteArray &device, const QAudioFormat &format);
    ~PULSEAudioOutput();

    qint64 write( const char *data, qint64 len );
    QIODevice* start(QIODevice* device);
    void stop();
    void reset();
    void suspend();
    void resume();
    int bytesFree() const;
    int periodSize() const;
    void setBufferSize(int value);
    int bufferSize() const;
    void setNotifyInterval(int milliSeconds);
    int notifyInterval() const;
    qint64 processedUSecs() const;
    qint64 elapsedUSecs() const;
    QAudio::Error error() const;
    QAudio::State state() const;
    QAudioFormat format() const;
    void setFormat(const QAudioFormat& fmt);

private slots:
    void userFeed();

private:
    bool open();
    void close();

    QByteArray m_device;
    QAudioFormat settings;
    QAudio::Error errorState;
    QAudio::State deviceState;
    QIODevice* audioSource;
    bool pullMode;
    QTimer* timer;
    QTime timeStamp;
    QTime clockTime;
    QTime writeTime;
    int intervalTime;
    char* audioBuffer;
    int bytesAvailable;
    int buffer_size;
    int period_size;
    unsigned int buffer_time;
    unsigned int period_time;
    qint64 totalTimeValue;
    qint64 saveProcessed;

    pa_sample_spec  params;
    pa_simple*      handle;
    pa_buffer_attr  attr;
    bool            connected;
    bool            writing;
    int             count;
    int             err;

    int dummyBuffer;
};

#endif
