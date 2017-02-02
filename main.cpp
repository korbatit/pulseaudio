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

#include <QtMultimedia>

#include "pulseaudio.h"

#include <qstringlist.h>
#include <qiodevice.h>
#include <qbytearray.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class PULSEAudioPlugin : public QAudioEnginePlugin
{
public:
    PULSEAudioPlugin();

    QStringList keys() const;

    QList<QByteArray> availableDevices(QAudio::Mode) const;
    QAbstractAudioInput* createInput(const QByteArray& device, const QAudioFormat& format);
    QAbstractAudioOutput* createOutput(const QByteArray& device, const QAudioFormat& format);
    QAbstractAudioDeviceInfo* createDeviceInfo(const QByteArray& device, QAudio::Mode mode);
};

PULSEAudioPlugin::PULSEAudioPlugin()
{
}

QStringList PULSEAudioPlugin::keys() const
{
    QStringList keys(QLatin1String("pulse"));

    return keys;
}

QList<QByteArray> PULSEAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    QList<QByteArray> devices;
    if (mode == QAudio::AudioOutput)
        devices.append("pulse");

    return devices;
}

QAbstractAudioInput* PULSEAudioPlugin::createInput(const QByteArray& device, const QAudioFormat& format)
{
    Q_UNUSED(device)
    Q_UNUSED(format)

    return 0;
}

QAbstractAudioOutput* PULSEAudioPlugin::createOutput(const QByteArray& device, const QAudioFormat& format)
{
    return (new PULSEAudioOutput(device,format));
}

QAbstractAudioDeviceInfo* PULSEAudioPlugin::createDeviceInfo(const QByteArray& device, QAudio::Mode mode)
{
    return (new PULSEAudioDeviceInfo(device,mode));
}

Q_EXPORT_STATIC_PLUGIN(PULSEAudioPlugin)
Q_EXPORT_PLUGIN2(pulseaudio, PULSEAudioPlugin)

QT_END_NAMESPACE

