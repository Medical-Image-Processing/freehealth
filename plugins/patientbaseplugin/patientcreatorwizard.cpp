/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *   Main developers : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#include "patientcreatorwizard.h"
#include "identitywidget.h"
#include "patientmodel.h"
#include "patientbase.h"
#include "constants_settings.h"

#include <coreplugin/icore.h>
#include <coreplugin/itheme.h>
#include <coreplugin/constants_icons.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/isettings.h>
#include <coreplugin/constants_tokensandsettings.h>

#include <utils/log.h>
#include <utils/global.h>
#include <translationutils/constants.h>
#include <translationutils/trans_current.h>
#include <translationutils/trans_titles.h>
#include <translationutils/trans_patient.h>

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>

using namespace Patients;
using namespace Trans::ConstantTranslations;

static inline Core::ITheme *theme() { return Core::ICore::instance()->theme(); }
static inline Core::IPatient *patient() { return Core::ICore::instance()->patient(); }
static inline Core::ISettings *settings() { return Core::ICore::instance()->settings(); }
static inline Patients::Internal::PatientBase *patientBase() { return Patients::Internal::PatientBase::instance(); }

PatientCreatorWizard::PatientCreatorWizard(QWidget *parent) :
    QWizard(parent)
{
    m_Page = new IdentityPage(this);
    addPage(m_Page);
    setWindowTitle(tr("New Patient"));
    QList<QWizard::WizardButton> layout;
    layout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton
            << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(layout);
    QPixmap pix = theme()->splashScreenPixmap("newpatient-wizard.png");
    setPixmap(QWizard::BackgroundPixmap, pix);
    setPixmap(QWizard::WatermarkPixmap, pix);
}

void PatientCreatorWizard::done(int r)
{
    if (r == QDialog::Rejected) {
        m_Saved = false;
        // has been anything entered already into the fields (data model is "dirty")?
        // -->then ask for confirmation
        bool reallyClose = !m_Page->isModified() ? true :
                                      Utils::yesNoMessageBox(tr("WARNING! You did not save this patient. "
                                                                "If you continue without saving, all changes will be lost."),
                                                             tr("Do you really want to close this dialog?"),
                                                             "", tr("Patient not saved"));
        if (reallyClose) {
            QDialog::done(r);
            if (Patients::PatientModel::activeModel())
                Patients::PatientModel::activeModel()->refreshModel();
        }
    } else if (r == QDialog::Accepted) {
        if (!validateCurrentPage())
            return;
        if (settings()->value(Core::Constants::S_PATIENTCHANGEONCREATION).toBool()) {
            Patients::PatientModel *m = Patients::PatientModel::activeModel();
            if (m) {
                QString uid = m_Page->lastInsertedUuid();
                m->setFilter("", "", uid, Patients::PatientModel::FilterOnUuid);
                m->setCurrentPatient(m->index(0,0));
            }
        }
        QDialog::done(r);
    }
}

/** \class IdentityPage
  * \brief Wizard page that asks for basic identity data like name, title, date of birth, gender, etc.
  */
IdentityPage::IdentityPage(QWidget *parent) :
    QWizardPage(parent)
{
    setObjectName("IdentityPage");
    setTitle(tr("Please enter the patient's identity."));
    m_Identity = new IdentityWidget(this, IdentityWidget::ReadWriteMode);
    m_Model = new PatientModel(this);
    m_Model->setFilter("", "", "__", PatientModel::FilterOnUuid);
    m_Model->emitPatientCreationOnSubmit(true);
    m_Model->insertRow(0);
    m_uuid = m_Model->index(0, Core::IPatient::Uid).data().toString();

    m_Identity->setPatientModel(m_Model);
    m_Identity->setCurrentIndex(m_Model->index(0,0));

    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(m_Identity, 0, 0);
    setLayout(layout);
}

bool IdentityPage::validatePage()
{
    if (!m_Identity->isIdentityValid())
        return false;

    // check duplicates
    if (patientBase()->isPatientExists(m_Identity->currentBirthName(),
                                       m_Identity->currentSecondName(),
                                       m_Identity->currentFirstName(),
                                       m_Identity->currentGender(),
                                       m_Identity->currentDateOfBirth())) {
        Utils::warningMessageBox(tr("Patient already exists"),
                                 tr("A patient with the same names, gender and date of birth "
                                    "already exists. You can not create duplicates."));
        return false;
    } else {
        // nearly duplicates (same names only)
        PatientModel *model = new PatientModel(this);
        model->setFilter(m_Identity->currentBirthName(), m_Identity->currentFirstName(), "", PatientModel::FilterOnFullName);
        if (model->rowCount() > 0) {
            QStringList names;
            int count = qMin(10, model->rowCount());
            for(int i=0; i < count; ++i)
                names << QString("%1 (%2; %3)")
                         .arg(model->data(model->index(i, Core::IPatient::FullName)).toString())
                         .arg(model->data(model->index(i, Core::IPatient::DateOfBirth)).toString())
                         .arg(model->data(model->index(i, Core::IPatient::Age)).toString());
            QString fullName = QString("%1 %2 %3<br />&nbsp;&nbsp;%4: %5<br />&nbsp;&nbsp;%6: %7")
                    .arg(m_Identity->currentBirthName())
                    .arg(m_Identity->currentSecondName())
                    .arg(m_Identity->currentFirstName())
                    .arg(tkTr(Trans::Constants::GENDER))
                    .arg(m_Identity->currentGender())
                    .arg(tkTr(Trans::Constants::DATE_OF_BIRTH))
                    .arg(m_Identity->currentDateOfBirth().toString(QLocale().dateFormat(QLocale::LongFormat))).simplified();
            QString msg = QString("%1.<br />"
                                  "<br />%2<br /><ul><li>%3</li></ul><br />%4")
                    .arg(tr("You are about to create the following  patient: %1")
                         .arg("<br /><br />&nbsp;&nbsp;<b>"+fullName+"</b>"))
                    .arg(tr("Patients with the same names exist in the database."))
                    .arg(names.join("</li><li>"))
                    .arg(tr("Do you really want to create this patient?"))
                    ;
            bool yes = Utils::yesNoMessageBox(tr("Patients of the same name exist"), msg);
            if (!yes)
                return false;
        }
        delete model;
    }
    // submit the new patient
    bool ok = true;
    connect(m_Model, SIGNAL(patientCreated(QString)), Patients::PatientModel::activeModel(), SIGNAL(patientCreated(QString)));
    if (m_Identity->submit()) {
        Patients::PatientModel::activeModel()->refreshModel();
        LOG("Patient successfully created");
    } else {
        LOG_ERROR("Unable to create patient. IdentityPage::validatePage()");
        ok = false;
    }
    return ok;
}

bool IdentityPage::isModified()
{
    //TODO: add photo + zip check
    return m_Identity->isModified();
}
