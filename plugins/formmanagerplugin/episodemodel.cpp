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
 *   Main Developper : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#include "episodemodel.h"
#include "episodebase.h"
#include "constants_db.h"
#include "constants_settings.h"
#include "episodedata.h"

#include <coreplugin/icore.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/itheme.h>
#include <coreplugin/iuser.h>
#include <coreplugin/constants_menus.h>
#include <coreplugin/constants_tokensandsettings.h>
#include <coreplugin/constants_icons.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <formmanagerplugin/formmanager.h>
#include <formmanagerplugin/iformitem.h>
#include <formmanagerplugin/iformitemspec.h>
#include <formmanagerplugin/iformitemdata.h>

#include <coreplugin/icore.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/isettings.h>
#include <coreplugin/itheme.h>

#include <utils/log.h>
#include <utils/global.h>
#include <translationutils/constanttranslations.h>
#include <extensionsystem/pluginmanager.h>

#include <QSqlTableModel>
#include <QSqlDatabase>
#include <QSqlQuery>

enum { base64MimeDatas = true  };

#ifdef DEBUG
enum {
    WarnDragAndDrop = false,
    WarnReparentItem = false,
    WarnDatabaseSaving = false,
    WarnFormAndEpisodeRetreiving = false
   };
#else
enum {
    WarnDragAndDrop = false,
    WarnReparentItem = false,
    WarnDatabaseSaving = false,
    WarnFormAndEpisodeRetreiving = false
   };
#endif


/**
  \todo When currentpatient change --> read all last episodes of forms and feed the patient model of the
  PatientDataRepresentation
*/

using namespace Form;
using namespace Internal;
using namespace Trans::ConstantTranslations;

static inline Form::Internal::EpisodeBase *episodeBase() {return Form::Internal::EpisodeBase::instance();}
//static inline Form::FormManager *formManager() { return Form::FormManager::instance(); }

static inline Core::IUser *user() {return Core::ICore::instance()->user();}
static inline Core::IPatient *patient() {return Core::ICore::instance()->patient();}

static inline Core::ISettings *settings()  { return Core::ICore::instance()->settings(); }
static inline Core::ITheme *theme()  { return Core::ICore::instance()->theme(); }
static inline Core::ActionManager *actionManager() { return Core::ICore::instance()->actionManager(); }
static inline ExtensionSystem::PluginManager *pluginManager() { return ExtensionSystem::PluginManager::instance(); }

namespace {

    /** \todo create an Utils::GenericTreeItem \sa Templates::TemplateModel, PMH::PmhCategoryModel */
    class TreeItem
    {
    public:
        TreeItem(TreeItem *parent = 0) :
                m_Parent(parent),
                m_IsEpisode(false),
                m_IsModified(false)
//                m_Datas(datas)
        {
//            setData(EpisodeModel::UserUuid, user()->uuid());
//            setIsEpisode(datas.value(EpisodeModel::IsEpisode).toBool());
        }
        ~TreeItem() { qDeleteAll(m_Children); }

        // Genealogy management
        TreeItem *child(int number) { return m_Children.value(number); }
        int childCount() const { return m_Children.count(); }
        int columnCount() const { return EpisodeModel::MaxData; }
        TreeItem *parent() { return m_Parent; }
        void setParent(TreeItem *parent) { m_Parent = parent; }
        bool appendChild(TreeItem *child)
        {
            if (!m_Children.contains(child))
                m_Children.append(child);
            return true;
        }
        bool insertChild(const int row, TreeItem *child)
        {
            if (row > m_Children.count())
                return false;
            m_Children.insert(row, child);
            return true;
        }
        int childNumber() const
        {
            if (m_Parent)
                return m_Parent->m_Children.indexOf(const_cast<TreeItem*>(this));
            return 0;
        }
        void sortChildren()
        {
            qSort(m_Children.begin(), m_Children.end(), TreeItem::lessThan);
        }

        // For category only tree
        int childCategoryCount() const
        {
            int n = 0;
            foreach(TreeItem *c, this->m_Children) {
                if (!c->isEpisode())
                    ++n;
            }
            return n;
        }

        TreeItem *categoryChild(int number)
        {
            QList<TreeItem *> cat;
            foreach(TreeItem *c, this->m_Children) {
                if (!c->isEpisode())
                    cat << c;
            }
            return cat.value(number);
        }

        int categoryChildNumber() const
        {
            if (m_Parent) {
                QList<TreeItem *> cat;
                foreach(TreeItem *c, m_Parent->m_Children) {
                    if (!c->isEpisode())
                        cat << c;
                }
                return cat.indexOf(const_cast<TreeItem*>(this));
            }
            return 0;
        }

        // For tree management
        void setIsEpisode(bool isEpisode) {m_IsEpisode = isEpisode; }
        bool isEpisode() const {return m_IsEpisode;}

        // For database management
        void setModified(bool state)
        {
            m_IsModified = state;
            if (!state)
                m_DirtyRows.clear();
        }
        bool isModified() const {return m_IsModified;}
//        void setNewlyCreated(bool state) {setData(EpisodeModel::IsNewlyCreated, state); }
//        bool isNewlyCreated() const {return data(EpisodeModel::IsNewlyCreated).toBool();}

        bool removeChild(TreeItem *child)
        {
            if (m_Children.contains(child)) {
                m_Children.removeAll(child);
                return true;
            }
            return false;
        }

        bool removeEpisodes()
        {
            foreach(TreeItem *item, m_Children) {
                if (item->isEpisode()) {
                    m_Children.removeAll(item);
                    delete item;
                }
            }
            return true;
        }

        // For data management
        QVariant data(const int column) const
        {
//            if (column==EpisodeModel::Label) {
//                if (!m_IsEpisode) {
//                    int nb = 0;
//                    for(int i = 0; i < m_Children.count(); ++i) {
//                        if (m_Children.at(i)->isEpisode())
//                            ++nb;
//                    }
//                    if (nb)
//                        return QString("%1 (%2)").arg(m_Datas.value(column).toString()).arg(nb);
//                }
//            }
//            return m_Datas.value(column, QVariant());
            return QVariant();
        }

        bool setData(int column, const QVariant &value)
        {
    //        qWarning()<< data(column) << value << (data(column)==value);
//            if (data(column)==value)
//                return true;
//            m_Datas.insert(column, value);
//            if (column==EpisodeModel::IsEpisode) {
//                m_IsEpisode=value.toBool();
//            }
//            m_IsModified = true;
//            if (!m_DirtyRows.contains(column))
//                m_DirtyRows.append(column);
            return true;
        }

        QVector<int> dirtyRows() const
        {
            return m_DirtyRows;
        }

        // For sort functions
        static bool lessThan(TreeItem *item1, TreeItem *item2)
        {
            // category goes first
            // then sort by name
            bool sameType = (((item1->isEpisode()) && (item2->isEpisode())) || ((!item1->isEpisode()) && (!item2->isEpisode())));
            if (sameType)
                return item1->data(EpisodeModel::Label).toString() < item2->data(EpisodeModel::Label).toString();
            return item2->isEpisode();
        }

    private:
        TreeItem *m_Parent;
        QList<TreeItem*> m_Children;
        QVector<int> m_DirtyRows;
        bool m_IsEpisode, m_IsModified;
        QHash<int, QVariant> m_Datas;
    };


}

namespace Form {
namespace Internal {

    EpisodeModelCoreListener::EpisodeModelCoreListener(Form::EpisodeModel *parent) :
            Core::ICoreListener(parent)
    {
        Q_ASSERT(parent);
        m_EpisodeModel = parent;
    }
    EpisodeModelCoreListener::~EpisodeModelCoreListener() {}

    bool EpisodeModelCoreListener::coreAboutToClose()
    {
        qWarning() << Q_FUNC_INFO;
        m_EpisodeModel->submit();
        return true;
    }

    EpisodeModelPatientListener::EpisodeModelPatientListener(Form::EpisodeModel *parent) :
            Core::IPatientListener(parent)
    {
        Q_ASSERT(parent);
        m_EpisodeModel = parent;
    }
    EpisodeModelPatientListener::~EpisodeModelPatientListener() {}

    bool EpisodeModelPatientListener::currentPatientAboutToChange()
    {
        qWarning() << Q_FUNC_INFO;
        m_EpisodeModel->submit();
        return true;
    }


//    EpisodeModelUserListener::EpisodeModelUserListener(Form::EpisodeModel *parent) :
//            UserPlugin::IUserListener(parent)
//    {
//        Q_ASSERT(parent);
//        m_EpisodeModel = parent;
//    }
//    EpisodeModelUserListener::~EpisodeModelUserListener() {}

//    bool EpisodeModelUserListener::userAboutToChange()
//    {
//        qWarning() << Q_FUNC_INFO;
//        m_EpisodeModel->submit();
//        return true;
//    }
//    bool EpisodeModelUserListener::currentUserAboutToDisconnect() {return true;}

class EpisodeModelPrivate
{
public:
    EpisodeModelPrivate(EpisodeModel *parent) :
        m_RootItem(0),
//        m_SynthesisItem(0),
        m_FormTreeCreated(false),
        m_ReadOnly(false),
        m_ActualEpisode(0),
        m_CoreListener(0),
        m_PatientListener(0),
        q(parent)
    {
    }

    ~EpisodeModelPrivate ()
    {
        qDeleteAll(m_Episodes);
        m_Episodes.clear();
        if (m_CoreListener) {
            pluginManager()->removeObject(m_CoreListener);
            delete m_CoreListener;
            m_CoreListener = 0;
        }
        if (m_PatientListener) {
            pluginManager()->removeObject(m_PatientListener);
            delete m_PatientListener;
            m_PatientListener = 0;
        }
    }

    bool isEpisode(TreeItem *item) { return (m_EpisodeItems.key(item, 0)!=0); }
    bool isForm(TreeItem *item) { return (m_FormItems.key(item, 0)!=0); }

    void createFormTree()
    {
        if (m_FormTreeCreated)
            return;

        if (m_RootItem) {
            delete m_RootItem;
            m_RootItem = 0;
        }

        // create root item
        m_RootItem = new TreeItem(0);
        m_FormUids.clear();

        // getting Forms
        if (WarnFormAndEpisodeRetreiving)
            LOG_FOR(q, "Getting Forms");

        // add the form synthesis item
//        m_SynthesisItem = new TreeItem(m_RootItem);
//        m_RootItem->appendChild(m_SynthesisItem);

        // create one item per form
        foreach(Form::FormMain *form, m_RootForm->flattenFormMainChildren()) {
            TreeItem *item = new TreeItem(0);
            m_FormItems.insert(form, item);
            m_FormUids << form->uuid();
        }
        // reparent items
        foreach(Form::FormMain *f, m_RootForm->flattenFormMainChildren()) {
            TreeItem *it = m_FormItems.value(f);
            if (f->formParent() != m_RootForm) {
                it->setParent(m_FormItems.value(f->formParent()));
                it->parent()->appendChild(it);
            } else {
                it->setParent(m_RootItem);
                m_RootItem->appendChild(it);
            }
            it->setModified(false);
        }
        m_FormTreeCreated = true;
    }

    /** Removes all episodes in TreeItems */
    void deleteEpisodes(TreeItem *item)
    {
        if (!item)
            return;
        if (isEpisode(item)) {
            item->parent()->removeChild(item);
            delete item;
            return;
        }
        item->removeEpisodes();
        int nb = item->childCount();
        for(int i = 0; i < nb; ++i) {
            deleteEpisodes(item->child(i));
        }
    }

    /** Clear the TreeItems of episode and repopulate with freshly extracted episodes from database */
    void refreshEpisodes()
    {
        // make sure that all actual episodes are saved into database
        if (!saveEpisode(m_ActualEpisode, m_ActualEpisode_FormUid))
            LOG_ERROR_FOR(q, "Unable to save actual episode");

        // delete old episodes
        deleteEpisodes(m_RootItem);
        m_ActualEpisode = 0;
        m_ActualEpisode_FormUid = "";
        qDeleteAll(m_Episodes);
        m_Episodes.clear();

        // get Episodes
        EpisodeBaseQuery query;
        query.setPatientUid(patient()->uuid());
        query.setValidEpisodes(true);
        query.setDeletedEpisodes(false);
        query.setFormUids(m_FormUids);
        m_Episodes = episodeBase()->getEpisodes(query);
        if (WarnFormAndEpisodeRetreiving)
            LOG_FOR(q, "Getting Episodes (refresh): " + QString::number(m_Episodes.count()));

        // create TreeItems and parent them
        for(int i = 0; i < m_Episodes.count(); ++i) {
            EpisodeData *episode = m_Episodes.at(i);
            // find episode's form parent
            TreeItem *formParent = 0;
            foreach(Form::FormMain *form, m_FormItems.keys()) {
                TreeItem *parent = m_FormItems.value(form);
                if (episode->data(EpisodeData::FormUuid).toString() == form->uuid()) {
                    formParent = parent;
                    break;
                }
            }
            if (!formParent) {
                qWarning() << "no valid formUid" << episode->data(EpisodeData::FormUuid).toString();
                continue;
            }
//                formParent = m_RootItem;
            TreeItem *item = new TreeItem(formParent);
            item->setParent(formParent);
            formParent->appendChild(item);

//            qWarning() << episode->data(EpisodeData::FormUuid).toString() << formParent;

            m_EpisodeItems.insert(episode, item);
        }
    }

    TreeItem *getItem(const QModelIndex &index) const
    {
        if (index.isValid()) {
            TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
            if (item)
                return item;
        }
        return m_RootItem;
    }

    /** \todo code here : limit memory usage by getting/saving XmlContent separately from base()->getEpisodes() */
    void getEpisodeContent(EpisodeData *episode)
    {
        if (episode->data(EpisodeData::Id).toInt()<0)
            return;
        if (episode->data(EpisodeData::IsXmlContentPopulated).toBool())
            return;
        episodeBase()->getEpisodeContent(episode);
    }

    QString createXmlEpisode(const QString &formUid)
    {
        /** \todo code here : use a QDomDocument */
        FormMain *form = m_RootForm->formMainChild(formUid);
        if (!form)
            return false;
//        bool formIsModified = false;

        QHash<QString, FormItem *> items;
        foreach(FormItem *it, form->flattenFormItemChildren()) {
            /** \todo check nested items ??? */
            if (it->itemDatas()) {
//                if (it->itemDatas()->isModified()) {
//                    qWarning() << it->uuid() << "is modified";
//                    formIsModified = true;
//                }
                items.insert(it->uuid(), it);
            }
        }

//        if (formIsModified) {
            // create the XML episode file
            QHash<QString, QString> datas;
            foreach(FormItem *it, items) {
                datas.insert(it->uuid(), it->itemDatas()->storableData().toString());
            }
            return Utils::createXml(Form::Constants::XML_FORM_GENERAL_TAG, datas, 2, false);
//        }

//        return QString();
    }

    bool saveEpisode(TreeItem *item, const QString &formUid)
    {
        if (!item)
            return true;
        if (formUid.isEmpty()) {
            LOG_ERROR_FOR("EpisodeModel", "No formUid");
            return false;
        }

        EpisodeData *episode = m_EpisodeItems.key(item);
        FormMain *form = 0;
        foreach(FormMain *f, m_FormItems.keys()) {
            if (f->uuid()==formUid) {
                form = f;
                break;
            }
        }

        if (episode && form) {
            episode->setData(EpisodeData::XmlContent, createXmlEpisode(formUid));
            episode->setData(EpisodeData::IsXmlContentPopulated, true);
            episode->setData(EpisodeData::Label, form->itemDatas()->data(0, IFormItemData::ID_EpisodeLabel));
            episode->setData(EpisodeData::UserDate, form->itemDatas()->data(0, IFormItemData::ID_EpisodeDate));
            LOG_FOR("EpisodeModel", "Save episode: " + episode->data(EpisodeData::Label).toString());
            if (!settings()->value(Core::Constants::S_ALWAYS_SAVE_WITHOUT_PROMPTING, true).toBool()) {
                bool yes = Utils::yesNoMessageBox(QCoreApplication::translate("EpisodeModel", "Save episode ?"),
                                                  QCoreApplication::translate("EpisodeModel", "The actual episode has been modified. Do you want to save changes in your database ?\n"
                                                     "Answering 'No' will cause definitve data lose."),
                                                  "", QCoreApplication::translate("EpisodeModel", "Save episode"));
                if (!yes) {
                    return false;
                }
            }

            // inform the patient model
            foreach(FormItem *it, form->flattenFormItemChildren()) {
                if (!it->itemDatas())
                    continue;
//                qWarning() << "Feeding patientModel data with" << it->patientDataRepresentation() << it->itemDatas()->data(it->patientDataRepresentation(), IFormItemData::ID_ForPatientModel);
                patient()->setValue(it->patientDataRepresentation(), it->itemDatas()->data(it->patientDataRepresentation(), IFormItemData::ID_ForPatientModel));
            }

            // save episode to database
            return episodeBase()->saveEpisode(episode);
        }
        return false;
    }

    void feedFormWithEpisodeContent(Form::FormMain *form, TreeItem *item, bool feedPatientModel = false)
    {
        EpisodeData *episode = m_EpisodeItems.key(item);
        feedFormWithEpisodeContent(form, episode, feedPatientModel);
    }

    void feedFormWithEpisodeContent(Form::FormMain *form, EpisodeData *episode, bool feedPatientModel = false)
    {
        getEpisodeContent(episode);
        const QString &xml = episode->data(EpisodeData::XmlContent).toString();
        if (xml.isEmpty()) {
            return;
        }

        // read the xml'd content
        QHash<QString, QString> datas;
        if (!Utils::readXml(xml, Form::Constants::XML_FORM_GENERAL_TAG, datas, false)) {
            LOG_ERROR_FOR(q, QString("Error while reading EpisodeContent"));
            return;
        }

        // put datas into the FormItems of the form
        // XML content ==
        // <formitemuid>value</formitemuid>
        QHash<QString, FormItem *> items;
        foreach(FormItem *it, form->flattenFormItemChildren()) {
            items.insert(it->uuid(), it);
        }

//        qWarning() << Q_FUNC_INFO << feedPatientModel << form->uuid();

        // feed the formitemdatas for this form and get the data for the patientmodel
        foreach(FormItem *it, items.values()) {
            if (!it) {
                qWarning() << "FormManager::activateForm :: ERROR : no item :" << items.key(it);
                continue;
            }
            if (!it->itemDatas())
                continue;

            it->itemDatas()->setStorableData(datas.value(it->uuid()));
            if (feedPatientModel) {
                qWarning() << "Feeding patientModel data with" << it->patientDataRepresentation() << it->itemDatas()->data(it->patientDataRepresentation(), IFormItemData::ID_ForPatientModel);
                patient()->setValue(it->patientDataRepresentation(), it->itemDatas()->data(it->patientDataRepresentation(), IFormItemData::ID_ForPatientModel));
            }
        }
    }

    void getLastEpisodesAndFeedPatientModel()
    {
        if (patient()->uuid().isEmpty())
            return;

        foreach(Form::FormMain *form, m_FormItems.keys()) {
            // test all children FormItem for patientDataRepresentation
            bool hasPatientDatas = false;
            foreach(Form::FormItem *item, form->flattenFormItemChildren()) {
                if (item->itemDatas()) {
                    if (item->patientDataRepresentation()!=-1) {
                        hasPatientDatas = true;
                        break;
                    }
                }
            }
            if (!hasPatientDatas)
                continue;

            // get the form's XML content for the last episode, feed it with the XML code
            TreeItem *formItem = m_FormItems.value(form);
            if (!formItem->childCount()) {
                continue;  // No episodes
            }

            // get last episode
            for(int i=0; i < m_Episodes.count(); ++i) {
                if (m_Episodes.at(i)->data(EpisodeData::FormUuid).toString()==form->uuid()) {
                    feedFormWithEpisodeContent(form, m_Episodes.at(i), true);
//                    qWarning() << "ACTIVATE EPISODE" << m_Episodes.at(i)->data(EpisodeData::Label).toString();
                }
            }
        }
    }

public:
    FormMain *m_RootForm;
    TreeItem *m_RootItem; //, *m_SynthesisItem;
    QString m_UserUuid, m_LkIds, m_CurrentPatient, m_CurrentForm;
    bool m_FormTreeCreated, m_ReadOnly;
    QStringList m_FormUids;

    QMap<Form::FormMain *, TreeItem *> m_FormItems;
    QMap<Form::Internal::EpisodeData *, TreeItem *> m_EpisodeItems;
    QList<Form::Internal::EpisodeData *> m_Episodes;

    /** \todo code here : remove m_ActualEpisode, m_ActualEpisode_FormUid */
    TreeItem *m_ActualEpisode;
    QString m_ActualEpisode_FormUid;

    EpisodeModelCoreListener *m_CoreListener;
    EpisodeModelPatientListener *m_PatientListener;

private:
    EpisodeModel *q;
};
}
}

EpisodeModel::EpisodeModel(FormMain *rootEmptyForm, QObject *parent) :
        QAbstractItemModel(parent), d(new Internal::EpisodeModelPrivate(this))
{
    Q_ASSERT(rootEmptyForm);
    setObjectName("EpisodeModel");
    d->m_RootForm = rootEmptyForm;

    // Autosave feature
    //    Core Listener
    d->m_CoreListener = new Internal::EpisodeModelCoreListener(this);
    pluginManager()->addObject(d->m_CoreListener);

    //    User Listener

    //    Patient change listener
    d->m_PatientListener = new Internal::EpisodeModelPatientListener(this);
    pluginManager()->addObject(d->m_PatientListener);

    init();
}

void EpisodeModel::init()
{
    d->m_UserUuid = user()->uuid();

    d->m_CurrentPatient = patient()->uuid();
    d->createFormTree();

    onUserChanged();

    // connect the save action
    Core::Command * cmd = actionManager()->command(Core::Constants::A_FILE_SAVE);
    connect(cmd->action(), SIGNAL(triggered()), this, SLOT(submit()));

    onPatientChanged();

    connect(Core::ICore::instance(), SIGNAL(databaseServerChanged()), this, SLOT(onCoreDatabaseServerChanged()));
    connect(user(), SIGNAL(userChanged()), this, SLOT(onUserChanged()));
    connect(patient(), SIGNAL(currentPatientChanged()), this, SLOT(onPatientChanged()));
}

void EpisodeModel::refreshFormTree()
{
    d->m_FormTreeCreated = false;
    d->createFormTree();
    d->refreshEpisodes();
    d->getLastEpisodesAndFeedPatientModel();
    reset();
}

EpisodeModel::~EpisodeModel()
{
    if (d) {
        delete d;
        d=0;
    }
}

void EpisodeModel::onCoreDatabaseServerChanged()
{
    d->m_FormTreeCreated = false;
    d->createFormTree();
    d->refreshEpisodes();
    d->getLastEpisodesAndFeedPatientModel();
    reset();
}

void EpisodeModel::onUserChanged()
{
    d->m_UserUuid = user()->uuid();
    /** \todo code here */
//    QList<int> ids = episodeBase()->retreivePractionnerLkIds(uuid);
//    d->m_LkIds.clear();
//    foreach(int i, ids)
//        d->m_LkIds.append(QString::number(i) + ",");
//    d->m_LkIds.chop(1);
//    d->refreshEpisodes();
}

void EpisodeModel::onPatientChanged()
{
    d->m_CurrentPatient = patient()->uuid();

    qWarning() << "CURRENT PATIENT" << d->m_CurrentPatient;

    d->refreshEpisodes();
    d->getLastEpisodesAndFeedPatientModel();
    reset();
}

QModelIndex EpisodeModel::index(int row, int column, const QModelIndex &parent) const
 {
     if (parent.isValid() && parent.column() != 0)
         return QModelIndex();

//     if (!parent.isValid())
//         return QModelIndex();

     TreeItem *parentItem = d->getItem(parent);
     TreeItem *childItem = 0;
     childItem = parentItem->child(row);
     if (childItem) { // && childItem != d->m_RootItem) {
         return createIndex(row, column, childItem);
     }
     return QModelIndex();
 }

QModelIndex EpisodeModel::parent(const QModelIndex &index) const
 {
     if (!index.isValid())
         return QModelIndex();

     TreeItem *childItem = d->getItem(index);
     TreeItem *parentItem = childItem->parent();

     if (parentItem == d->m_RootItem)
         return QModelIndex();

     return createIndex(parentItem->childNumber(), 0, parentItem);
 }

int EpisodeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *item = d->getItem(parent);
    if (item) {
        return item->childCount();
    }
    return 0;
}

int EpisodeModel::columnCount(const QModelIndex &) const
{
    return MaxData;
}

QVariant EpisodeModel::data(const QModelIndex &item, int role) const
{
    if (!item.isValid())
        return QVariant();

    if (item.column() == EmptyColumn1)
        return QVariant();
    if (item.column() == EmptyColumn2)
        return QVariant();

    TreeItem *it = d->getItem(item);
    if (it==d->m_RootItem)
        return QVariant();

//    if (it==d->m_SynthesisItem) {
//        switch (role) {
//        case Qt::DisplayRole:
//        case Qt::EditRole:
//            if (item.column() == FormUuid)
//                return Constants::PATIENTSYNTHESIS_UUID;
//            if (item.column() == Label)
//                return QApplication::translate(Constants::FORM_TR_CONTEXT, Constants::SHOWPATIENTSYNTHESIS_TEXT);
//            break;
//        case Qt::FontRole:
//        {
//            QFont bold;
//            bold.setBold(true);
//            return bold;
//        }
//        case Qt::DecorationRole:
//            return theme()->icon(Core::Constants::ICONPATIENTSYNTHESIS);
//        }
//        return QVariant();
//    }

    EpisodeData *episode = d->m_EpisodeItems.key(it, 0);
    FormMain *form = d->m_FormItems.key(it, 0);

    switch (role)
    {
    case Qt::EditRole :
    case Qt::DisplayRole :
    {
        switch (item.column()) {
        case Label:
            if (episode)
                return episode->data(EpisodeData::Label);
            if (form)
                return form->spec()->label();
            break;
        case IsValid:
            if (episode)
                return episode->data(EpisodeData::IsValid);
            if (form)
                return true;
            break;
            //           case  Summary,
            //           case  FullContent,
            //           case  Id,
            //           case  UserUuid,
            //           case  PatientUuid,
        case  FormUuid:
            if (episode)
                return episode->data(EpisodeData::FormUuid);
            if (form)
                return form->uuid();
            break;
//        case IsNewlyCreated:
        case IsEpisode:
            return (episode!=0);
        case XmlContent:
            if (episode) {
                if (!episode->data(EpisodeData::IsXmlContentPopulated).toBool())
                    d->getEpisodeContent(episode);
                return episode->data(EpisodeData::XmlContent);
            }
            break;
        }
    }
    case Qt::ToolTipRole :
    {
        switch (item.column()) {
        case Label:
            if (episode)
                return QString("<p align=\"right\">%1&nbsp;-&nbsp;%2<br /><span style=\"color:gray;font-size:9pt\">%3</span></p>")
                        .arg(episode->data(EpisodeData::UserDate).toDate().toString(settings()->value(Constants::S_EPISODEMODEL_DATEFORMAT, "dd MMM yyyy").toString()).replace(" ", "&nbsp;"))
                        .arg(episode->data(EpisodeData::Label).toString().replace(" ", "&nbsp;"))
                        .arg(user()->value(Core::IUser::FullName).toString() + "<br/>" +
                             tr("Created: ") +  episode->data(EpisodeData::CreationDate).toDateTime().toString(QLocale().dateTimeFormat(QLocale::ShortFormat)));
            if (form)
                return form->spec()->label();
            break;
        }
    }
    case Qt::ForegroundRole :
    {
        if (episode) {
            return QColor(settings()->value(Constants::S_EPISODEMODEL_EPISODE_FOREGROUND, "darkblue").toString());
        } else {
            /** \todo remove this */
            Form::FormMain *form = d->m_FormItems.key(it, 0);
            if (form) {
                if (form->episodePossibilities()==FormMain::UniqueEpisode)
                    return QColor("red");
            }
            // End remove
            return QColor(settings()->value(Constants::S_EPISODEMODEL_FORM_FOREGROUND, "#000").toString());
        }
    }
    case Qt::FontRole :
    {
        if (form) {
            QFont bold;
            bold.setBold(true);
            return bold;
        }
        return QFont();
    }
    case Qt::DecorationRole :
    {
        if (form) {
            QString icon = form->spec()->value(FormItemSpec::Spec_IconFileName).toString();
            icon.replace(Core::Constants::TAG_APPLICATION_THEME_PATH, settings()->path(Core::ISettings::SmallPixmapPath));
            if (QFileInfo(icon).isRelative())
                icon.append(qApp->applicationDirPath());
            return QIcon(icon);
        }
    }
    }
    return QVariant();
}

bool EpisodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (d->m_ReadOnly)
        return false;

    if (!index.isValid())
        return false;

    TreeItem *it = d->getItem(index);
    if (it==d->m_RootItem)
        return false;

    EpisodeData *episode = d->m_EpisodeItems.key(it, 0);

    if ((role==Qt::EditRole) || (role==Qt::DisplayRole)) {
        if (episode) {
            switch (index.column()) {
            case Label: episode->setData(EpisodeData::Label, value); break;
            case Date: episode->setData(EpisodeData::UserDate, value); break;
            case IsValid: episode->setData(EpisodeData::IsValid, value); break;
            case FormUuid: episode->setData(EpisodeData::FormUuid, value); break;
            case XmlContent: episode->setData(EpisodeData::XmlContent, value); episode->setData(EpisodeData::IsXmlContentPopulated, value); break;
            }
        }

        Q_EMIT dataChanged(index, index);
    }
    return true;
}

Qt::ItemFlags EpisodeModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

/** Not implemented */
QVariant EpisodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

/** Insert new episodes to the \e parent. The parent must be a form. */
bool EpisodeModel::insertRows(int row, int count, const QModelIndex &parent)
{
//    qWarning() << "insertRows" << row << count << parent.data();
    if (d->m_ReadOnly)
        return false;

    if (!parent.isValid())
        return false;

    TreeItem *parentItem = d->getItem(parent);
    if (!parentItem)
        return false;

    FormMain *form = formForIndex(parent);
    if (!form)
        return false;
    const QString &formUid = form->uuid();

    beginInsertRows(parent, row, row + count);
    for(int i = 0; i < count; ++i) {
        // create the episode
        Internal::EpisodeData *episode = new Internal::EpisodeData;
        episode->setData(Internal::EpisodeData::Label, tr("New episode"));
        episode->setData(Internal::EpisodeData::FormUuid, formUid);
        episode->setData(Internal::EpisodeData::UserCreatorUuid, user()->uuid());
        episode->setData(Internal::EpisodeData::PatientUuid, patient()->uuid());
        episode->setData(Internal::EpisodeData::CreationDate, QDateTime::currentDateTime());
        episode->setData(Internal::EpisodeData::UserDate, QDateTime::currentDateTime());
        episode->setData(Internal::EpisodeData::IsValid, true);
        /** \todo code here : create an episode modification to store the user creator ??? */

        // create the tree item
        TreeItem *it = new TreeItem(parentItem);
        parentItem->insertChild(row+i, it);

        // link episode/item
        d->m_EpisodeItems.insert(episode, it);
        d->m_Episodes.append(episode);
    }
    endInsertRows();
    return true;
}

/** Not implemented */
bool EpisodeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    //    qWarning() << "removeRows" << row << count;
    if (d->m_ReadOnly)
        return false;

    return true;
}

/** Return true is the \e index is an episode. */
bool EpisodeModel::isEpisode(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    TreeItem *it = d->getItem(index);
    if (it==d->m_RootItem)
        return false;

    EpisodeData *episode = d->m_EpisodeItems.key(it, 0);

    return (episode);
}

/** Return true is the \e index only owns one unique episode. It is supposed that the \e index points to a form */
bool EpisodeModel::isUniqueEpisode(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    TreeItem *it = d->getItem(index);
    if (it==d->m_RootItem)
        return false;

    FormMain *form = d->m_FormItems.key(it, 0);
    if (form)
        return form->episodePossibilities()==FormMain::UniqueEpisode;

    return false;
}

/** Return true is the \e index does not own episodes. It is supposed that the \e index points to a form */
bool EpisodeModel::isNoEpisode(const QModelIndex &index)
{
    if (!index.isValid())
        return false;
    TreeItem *it = d->getItem(index);
    if (it==d->m_RootItem)
        return false;

    FormMain *form = d->m_FormItems.key(it, 0);
    if (form)
        return form->episodePossibilities()==FormMain::NoEpisode;

    return false;
}

/** Define the whole model read mode */
void EpisodeModel::setReadOnly(const bool state)
{
    d->m_ReadOnly = state;
}

/** Return true if the whole model is in a read only mode */
bool EpisodeModel::isReadOnly() const
{
    return d->m_ReadOnly;
}

/** Return true if the model has unsaved data */
bool EpisodeModel::isDirty() const
{
    return false;
}

/** Return the Form::FormMain pointer corresponding to the \e index. The returned pointer must not be deleted */
Form::FormMain *EpisodeModel::formForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;
    QModelIndex idx = index;
    while (idx.isValid()) {
        TreeItem *it = d->getItem(idx);
        if (it==d->m_RootItem)
            return 0;
        FormMain *form = d->m_FormItems.key(it, 0);
        if (form)
            return form;
        idx = idx.parent();
    }
    return 0;
}

static QModelIndex formIndex(const QString &formUid, const QModelIndex &parent, const Form::EpisodeModel *model)
{
    // Test parent
    if (model->isForm(parent)) {
        if (parent.data().toString()==formUid) {
            return model->index(parent.row(), 0, parent.parent());
        }
    }
    // Test its children
    for(int i = 0; i < model->rowCount(parent); ++i) {
        QModelIndex item = model->index(i, EpisodeModel::FormUuid, parent);
        QModelIndex ret = formIndex(formUid, item, model);
        if (ret.isValid())
            return model->index(ret.row(), 0, ret.parent());
    }
    return QModelIndex();
}

/** Return the QModelIndex corresponding to the form with the specified \e formUid, or return an invalid index. */
QModelIndex EpisodeModel::indexForForm(const QString &formUid) const
{
    for(int i = 0; i < rowCount(); ++i) {
        QModelIndex ret = formIndex(formUid, index(i, EpisodeModel::FormUuid), this);
        if (ret.isValid()) {
            return ret;
        }
    }
    return QModelIndex();
}

/** Save the whole model. \sa isDirty() */
bool EpisodeModel::submit()
{
    // No active patient ?
    if (patient()->uuid().isEmpty())
        return false;

    // save actual episode if needed
    if (d->m_ActualEpisode) {
        if (!d->saveEpisode(d->m_ActualEpisode, d->m_ActualEpisode_FormUid)) {
            LOG_ERROR("Unable to save actual episode before editing a new one");
        }
    }
    return true;
}

bool EpisodeModel::activateEpisode(const QModelIndex &index, const QString &formUid) //, const QString &xmlcontent)
{
    qWarning() << "activateEpisode";
    // submit actual episode
    if (!d->saveEpisode(d->m_ActualEpisode, d->m_ActualEpisode_FormUid)) {
        LOG_ERROR("Unable to save actual episode before editing a new one");
    }

    if (!index.isValid()) {
        d->m_ActualEpisode = 0;
        return false;
    }

    // stores the actual episode id
    TreeItem *it = d->getItem(index);
    if (it==d->m_RootItem)
        return false;

    EpisodeData *episode = d->m_EpisodeItems.key(it, 0);
    FormMain *form = formForIndex(index);
    if (!episode) {
        d->m_ActualEpisode = 0;
        return false;
    }
    d->m_ActualEpisode = it;
    d->m_ActualEpisode_FormUid = formUid;

    // clear actual form and fill episode datas
    if (!form)
        return false;
    form->clear();
    form->itemDatas()->setData(0, episode->data(EpisodeData::UserDate), IFormItemData::ID_EpisodeDate);
    form->itemDatas()->setData(0, episode->data(EpisodeData::Label), IFormItemData::ID_EpisodeLabel);
    const QString &username = user()->fullNameOfUser(episode->data(EpisodeData::UserCreatorUuid)); //value(Core::IUser::FullName).toString();
    if (username.isEmpty())
        form->itemDatas()->setData(0, tr("No user"), IFormItemData::ID_UserName);
    else
        form->itemDatas()->setData(0, username, IFormItemData::ID_UserName);

    qWarning() << "EpisodeModel::activateEpisode" << formUid;


    /** \todo move this part into a specific member of the private part */
    d->getEpisodeContent(episode);
    const QString &xml = episode->data(EpisodeData::XmlContent).toString();
    if (xml.isEmpty())
        return true;

    // read the xml'd content
    QHash<QString, QString> datas;
    if (!Utils::readXml(xml, Form::Constants::XML_FORM_GENERAL_TAG, datas, false)) {
        LOG_ERROR(QString("Error while reading EpisodeContent %2:%1").arg(__LINE__).arg(__FILE__));
        return false;
    }

    // put datas into the FormItems of the form
    // XML content ==
    // <formitemuid>value</formitemuid>
    QHash<QString, FormItem *> items;
    foreach(FormItem *it, form->flattenFormItemChildren()) {
        /** \todo check nested items */
        items.insert(it->uuid(), it);
    }

    foreach(const QString &s, datas.keys()) {
        FormItem *it = items.value(s, 0);
        if (!it) {
            qWarning() << "FormManager::activateForm :: ERROR : no item :" << s;
            continue;
        }
        if (it->itemDatas())
            it->itemDatas()->setStorableData(datas.value(s));
        else
            qWarning() << "FormManager::activateForm :: ERROR : no itemData :" << s;
    }
    return true;
}

/** Save the episode pointed by the \e index to the database. */
bool EpisodeModel::saveEpisode(const QModelIndex &index, const QString &formUid)
{
    return d->saveEpisode(d->getItem(index), formUid);
}


