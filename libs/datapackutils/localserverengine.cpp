/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developpers :                                                    *
 *       Eric MAEKER, MD <eric.maeker@gmail.com>                           *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#include "localserverengine.h"
#include "servermanager.h"
#include "core.h"

#include <utils/global.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

using namespace DataPack;
using namespace Internal;

static inline DataPack::Core &core() {return DataPack::Core::instance();}

LocalServerEngine::LocalServerEngine(IServerManager *parent) :
    IServerEngine(parent)
{
}

ServerManager *LocalServerEngine::serverManager()
{
    return qobject_cast<ServerManager*>(parent());
}

bool LocalServerEngine::managesServer(const Server &server)
{
    return server.nativeUrl().startsWith("file://");
}

void LocalServerEngine::addToDownloadQueue(const ServerEngineQuery &query)
{
    m_queue.append(query);
}

int LocalServerEngine::downloadQueueCount() const
{
    return m_queue.count();
}

bool LocalServerEngine::startDownloadQueue()
{
    for(int i = 0; i < m_queue.count(); ++i) {
        const ServerEngineQuery &query = m_queue.at(i);
        Server *server = query.server;
        if (query.downloadDescriptionFiles) {
            // Read the local server config
            server->fromXml(Utils::readTextFile(server->url(Server::ServerConfigurationFile), Utils::DontWarnUser));
            // Read the local pack config
            for(int j = 0; j < server->content().packDescriptionFileNames().count(); ++j) {
                Pack p;
                p.fromXmlFile(server->url(Server::PackDescriptionFile, server->content().packDescriptionFileNames().at(j)));
                serverManager()->registerPack(*server, p);
            }
        }
        if (query.downloadPackFile) {
            Pack *pack = query.pack;
            QString url = server->url(Server::PackFile, pack->serverFileName());
            QFileInfo local(url);
            if (local.exists()) {
                // copy pack to datapack core persistentCachePath
                QString newPath = core().persistentCachePath() + QDir::separator() + pack->uuid();
                QDir newDir(newPath);
                if (!newDir.exists()) {
                    QDir().mkpath(newPath);
                }
                QFile::copy(local.absoluteFilePath(), newPath +  QDir::separator() + local.fileName());

                // copy pack XML config
                QString orig = pack->localFileName();
                QFile::copy(orig, newPath +  QDir::separator() + "packconfig.xml");


            }
        }
    }
    m_queue.clear();
    return true;
}
