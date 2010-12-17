/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2010 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
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
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#include "pubmeddownloader.h"

#include <utils/log.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace Utils;
//
//  Get textual summary of publication (pubmed)
//  http://www.ncbi.nlm.nih.gov/pubmed/8148870?dopt=docsum&format=text
//

static const char * REFERENCE_URL = "http://www.ncbi.nlm.nih.gov/pubmed/%1?dopt=docsum&format=text";
static const char * ABSTRACT_URL  = "http://www.ncbi.nlm.nih.gov/pubmed/%1?dopt=Abstract&format=text";


PubMedDownloader::PubMedDownloader(QObject *parent) :
        QObject(parent), manager(0), m_DownloadingReferences(false)
{
    manager = new QNetworkAccessManager(this);
}

bool PubMedDownloader::setFullLink(const QString &link)
{
    m_Reference.clear();
    m_Abstract.clear();
    m_Pmid.clear();
    if (!link.startsWith("http://www.ncbi.nlm.nih.gov/pubmed/")) {
        Utils::Log::addError(this, tr("Wrong PubMed link %1").arg(link));
        return false;
    }
    m_Pmid = link;
    m_Pmid = m_Pmid.remove("http://www.ncbi.nlm.nih.gov/pubmed/");
    if (m_Pmid.contains("?")) {
        m_Pmid = m_Pmid.mid(m_Pmid.indexOf("?"));
    }
    if (m_Pmid.contains(QRegExp("\\D"))) {
        Utils::Log::addError(this, tr("Wrong PubMed link %1. Extract PMID %2").arg(link).arg(m_Pmid));
        m_Pmid.clear();
        return false;
    }
    return true;
}

void PubMedDownloader::startDownload()
{
    if (m_Pmid.isEmpty()) {
        Q_EMIT downloadFinished();
        return;
    }
    qWarning() << "PubMedDownloader start" << QString(REFERENCE_URL).arg(m_Pmid);
    m_Reference.clear();
    m_Abstract.clear();
    manager->disconnect();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(referencesFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(QUrl(QString(REFERENCE_URL).arg(m_Pmid))));
}

void PubMedDownloader::referencesFinished(QNetworkReply *reply)
{
    static int nb = 0;
    qWarning() << "PubMedDownloader Reference" << reply->url();
    m_Reference = reply->readAll();
    int b = m_Reference.indexOf("<pre>\n1: ") + 9;
    int e = m_Reference.indexOf("</pre>", b);
    m_Reference = m_Reference.mid(b, e-b);
    m_Reference.replace("&lt;", "<");
    m_Reference.replace("&gt;", ">");
    manager->disconnect();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(abstractFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(QUrl(QString(ABSTRACT_URL).arg(m_Pmid))));
}

void PubMedDownloader::abstractFinished(QNetworkReply *reply)
{
    static int nb = 0;
    qWarning() << "PubMedDownloader Abstract" << reply->url();
    m_Abstract = reply->readAll();
    int b = m_Abstract.indexOf("<pre>\n1. ") + 9;
    int e = m_Abstract.indexOf("</pre>", b);
    m_Abstract.replace("&lt;", "<");
    m_Abstract.replace("&gt;", ">");
    m_Abstract = m_Abstract.mid(b, e-b);
    manager->disconnect();
    Q_EMIT downloadFinished();
}
