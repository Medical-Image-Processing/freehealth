/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2011 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  The FreeAccount plugins are free, open source FreeMedForms' plugins.   *
 *  (C) 2010-2011 by Pierre-Marie Desombre, MD <pm.desombre@medsyn.fr>     *
 *  and Eric Maeker, MD <eric.maeker@gmail.com>                            *
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
 *  Main Developpers : Pierre-Marie DESOMBRE <pm.desombre@medsyn.fr>,      *
 *                     Eric MAEKER, <eric.maeker@gmail.com>                *
 *  Contributors :                                                         *
 *      NAME <MAIL@ADRESS>                                                 *
 ***************************************************************************/
#include "findReceiptsValues.h"
#include <QSqlQuery>
#include <QSqlTableModel>

using namespace AccountDB;
using namespace Constants;

findReceiptsValues::findReceiptsValues(QWidget * parent):QDialog(parent){
  ui = new Ui::findValueDialog;
  ui->setupUi(this);
  ui->nextButton->hide();
  ui->nameRadioButton->setChecked(true);
  MedicalProcedureModel model(parent);
  m_db = QSqlDatabase::database(Constants::DB_ACCOUNTANCY);
  qDebug() << __FILE__ << QString::number(__LINE__)   ;
  fillComboCategories();
  qDebug() << __FILE__ << QString::number(__LINE__)   ;
  initialize();
  qDebug() << __FILE__ << QString::number(__LINE__)   ;
  QString comboValue = ui->comboBoxCategories->currentText().trimmed();
  emit fillListViewValues(comboValue);
  qDebug() << __FILE__ << QString::number(__LINE__)   ;
  connect(ui->comboBoxCategories,SIGNAL(activated(const QString&)),this,SLOT(fillListViewValues(const QString&)));
  connect(ui->tableViewOfValues,SIGNAL(pressed(const QModelIndex&)),this,SLOT(chooseValue(const QModelIndex&)));
  connect(ui->listChoosenWidget,SIGNAL(itemClicked(QListWidgetItem *)),this,SLOT(supprItemChoosen(QListWidgetItem *)));
  connect(ui->nextButton,SIGNAL(pressed()),this,SLOT(showNext()));
}

findReceiptsValues::~findReceiptsValues(){
  delete m_xmlParser;
  //delete m_rbm;
  //delete m_mpmodel;
  ui->listChoosenWidget->clear();
}

void findReceiptsValues::initialize(){
    m_xmlParser = new xmlCategoriesParser;
    //m_rbm = new receiptsManager;
    //m_mpmodel = new AccountDB::MedicalProcedureModel(this);
    if(m_hashValuesChoosen.size()>0){
        m_hashValuesChoosen.clear();
        }
}

void findReceiptsValues::clear(){
    ui->listChoosenWidget->clear();
    m_hashValuesChoosen.clear();
}

void findReceiptsValues::fillComboCategories(){
    QStringList choiceList ;
    /*QHash<QString,QString> hashCategories = m_xmlParser->readXmlFile()[0];
    choiceList = hashCategories.value("typesOfReceipts").split(",");
    MedicalProcedureModel *model = new MedicalProcedureModel(this);
    int MPRows = model->rowCount(QModelIndex());
    qDebug() << __FILE__ << QString::number(__LINE__) << " rowCount =" << QString::number(MPRows) ;
    for (int i = 0; i < MPRows; i += 1)
    {
        QString typeData = model->data(model->index(i,MP_TYPE)).toString();
        if(!choiceList.contains(typeData)){
           choiceList << typeData;
           }
    }*/
    QSqlQuery q(m_db);
    const QString req = QString("SELECT %1 FROM %2").arg("TYPE","medical_procedure");
    if (!q.exec(req))
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << q.lastError().text() ;
        }
    while (q.next())
    {
    	QString type = q.value(0).toString();
    	choiceList << type;
        }
    choiceList.removeDuplicates();
    ui->comboBoxCategories->setEditable(true);
    ui->comboBoxCategories->setInsertPolicy(QComboBox::NoInsert);
    ui->comboBoxCategories->addItems(choiceList);
}

/*void findReceiptsValues::fillListViewValues(const QString & comboItem){
    QString filter = QString("%1 LIKE '%%2%'").arg("TYPE",comboItem);
    if (!((itemModel = new QStandardItemModel(this)) == NULL) )
    {
        itemModel->clear();
        }
    QVariant act = QVariant(trUtf8("Name"));
    QVariant value = QVariant(trUtf8("Value"));
    model->setFilter(filter);
    int count =   model->rowCountWithFilter(QModelIndex(),filter);
    for (int i = 0; i < count; i += 1)
    {
    	QString name = model->dataWithFilter(model->index(i,MP_NAME),Qt::DisplayRole,filter).toString();
    	QString value = model->dataWithFilter(model->index(i,MP_AMOUNT),Qt::DisplayRole,filter).toString();
    	qDebug() << __FILE__ << QString::number(__LINE__) << " names =" << name ;
    	QStandardItem *itemName = new QStandardItem(name);
    	QStandardItem *itemValue = new QStandardItem(value);
    	QList<QStandardItem*> list;
    	list << itemName << itemValue;
    	itemModel->appendRow(list);
        }
    model->setFilter("");
    if (!itemModel->setHeaderData(0,Qt::Horizontal,act,Qt::EditRole))
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
    	  }
    if (!itemModel->setHeaderData(1,Qt::Horizontal,value,Qt::EditRole)	)
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
        } 
    ui->tableViewOfValues->setModel(itemModel);
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
    if (tableViewIsFull(ui->tableViewOfValues->model()))
    {
    	  enableShowNextTable();
        }
}*/

void findReceiptsValues::fillListViewValues(const QString & comboItem){
    const QString baseName = trUtf8("medical_procedure");
    const QString strItem = comboItem.trimmed();
    const QString name = trUtf8("NAME");
    const QString amount = trUtf8("AMOUNT");
    const QString type = trUtf8("TYPE");
    QString filter = QString("WHERE %1 = '%2'").arg(type,strItem);
    QString req = QString("SELECT %1,%2 FROM %3 ").arg(name,amount,baseName )+filter;
    QStandardItemModel *model = new QStandardItemModel(0,2,this);
    int row = 0;
    QSqlQuery q(m_db);
    if (!q.exec(req))
    {
    	 qWarning() << __FILE__ << QString::number(__LINE__) 
    	                        << "Error __FILE__"+QString::number(__LINE__)+q.lastError().text() ; 
        }
    while (q.next())
    {
    	QString n = q.value(0).toString();
    	QString a = q.value(1).toString();
    	//qDebug() << __FILE__ << QString::number(__LINE__) << " n and a	= " << n << a;
    	model->insertRows(row,1,QModelIndex());
    	model->setData(model->index(row,0),n,Qt::EditRole);
        model->setData(model->index(row,1),a,Qt::EditRole);
        model->submit();
        //qDebug() << __FILE__ << QString::number(__LINE__) << " model data =" << model->data(model->index(row,0),Qt::DisplayRole).toString();
        ++row;
        //qDebug() << __FILE__ << QString::number(__LINE__) << " rows =" << QString::number(row) ;
        }
    ui->tableViewOfValues->setModel(model);
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
    /*if (tableViewIsFull(ui->tableViewOfValues->model()))
    {
    	  enableShowNextTable();
        }*/
}

void findReceiptsValues::chooseValue(const QModelIndex& index){
    QModelIndex inIndex(index);
    //get datas
    int row = inIndex.row();
    QAbstractItemModel * model = ui->tableViewOfValues->model();
    QModelIndex indexData = model->index(row,0,QModelIndex());
    QModelIndex indexAmount = model->index(row,1,QModelIndex());
    QString data = model->data(indexData,Qt::DisplayRole).toString();//NAME
    QString amount = model->data(indexAmount,Qt::DisplayRole).toString();//AMOUNT
    qDebug() << __FILE__ << QString::number(__LINE__) << " data = " << data;
    ui->listChoosenWidget->addItem(data);
    m_hashValuesChoosen.insert(data,amount);
}

void findReceiptsValues::supprItemChoosen(QListWidgetItem * item){
    qDebug() << __FILE__ << QString::number(__LINE__) << " item = " << item->text();
    QString dataToRemove = item->data(Qt::DisplayRole).toString();
    m_hashValuesChoosen.remove(dataToRemove);
    delete item;
}

QHash<QString,QString> findReceiptsValues::getChoosenValues(){
    return m_hashValuesChoosen;
}

/*void findReceiptsValues::on_lineEditFilter_textChanged(const QString & text){
    if (!((itemModel = new QStandardItemModel(this)) == NULL) )
    {
        itemModel->clear();
        }
    QString comboChoice = ui->comboBoxCategories->currentText();
    QString filterText = ""+text+"%";
    QString filter = QString("%1 LIKE '%2' AND %3 LIKE '%4'").arg("TYPE",comboChoice,"NAME",filterText);
    QVariant act = QVariant(trUtf8("Name"));
    QVariant value = QVariant(trUtf8("Value"));
    model->setFilter(filter);
    int count =   model->rowCountWithFilter(QModelIndex(),filter);
    for (int i = 0; i < count; i += 1)
    {
    	QString name = model->dataWithFilter(model->index(i,MP_NAME),Qt::DisplayRole,filter).toString();
    	qDebug() << __FILE__ << QString::number(__LINE__) << " names =" << name ;
    	QString value = model->dataWithFilter(model->index(i,MP_AMOUNT),Qt::DisplayRole,filter).toString();
    	QStandardItem *itemName = new QStandardItem(name);
    	QStandardItem *itemValue = new QStandardItem(value);
    	QList<QStandardItem*> list;
    	list << itemName << itemValue;
    	itemModel->appendRow(list);
        }
    model->setFilter("");
    if (!itemModel->setHeaderData(0,Qt::Horizontal,act,Qt::EditRole))
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
    	  }
    if (!itemModel->setHeaderData(1,Qt::Horizontal,value,Qt::EditRole)	)
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
        } 
    ui->tableViewOfValues->setModel(itemModel);
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
    if (tableViewIsFull(ui->tableViewOfValues->model()))
    {
    	  enableShowNextTable();
        }
}*/

void findReceiptsValues::on_lineEditFilter_textChanged(const QString & text){
    QString comboChoice = ui->comboBoxCategories->currentText();
    QString filterText ;
    QString filter; 
    const QString baseName = trUtf8("medical_procedure");
    const QString name = trUtf8("NAME");
    const QString amount = trUtf8("AMOUNT");
    const QString type = trUtf8("TYPE");
    if (ui->nameRadioButton->isChecked())
    {
    	  filterText = ""+text+"%";
    	  filter = QString("WHERE %1 LIKE '%2' AND %3 LIKE '%4'").arg("TYPE",comboChoice,"NAME",filterText); 
        }
    else if (ui->abstractRadioButton->isChecked())
    {
    	  filterText = "%"+text+"%";
    	  filter = QString("WHERE %1 LIKE '%2' AND %3 LIKE '%4'").arg("TYPE",comboChoice,"ABSTRACT",filterText); 
        }
    else{
    	QMessageBox::warning(0,trUtf8("Warning"),trUtf8("Check a radioButton."),QMessageBox::Ok);
    }
    QString req = QString("SELECT %1,%2 FROM %3 ").arg(name,amount,baseName )+filter;
    QStandardItemModel *model = new QStandardItemModel(1,2,this);
    int row = 0;
    QSqlQuery q(m_db);
    if (!q.exec(req))
    {
    	 qWarning() << __FILE__ << QString::number(__LINE__) 
    	                        << "Error __FILE__"+QString::number(__LINE__)+q.lastError().text() ; 
        }
    while (q.next())
    {
    	QString n = q.value(0).toString();
    	QString a = q.value(1).toString();
    	//qDebug() << __FILE__ << QString::number(__LINE__) << " n and a	= " << n << a;
    	model->insertRows(row,1,QModelIndex());
    	model->setData(model->index(row,0),n,Qt::EditRole);
        model->setData(model->index(row,1),a,Qt::EditRole);
        model->submit();
        //qDebug() << __FILE__ << QString::number(__LINE__) << " model data =" << model->data(model->index(row,0),Qt::DisplayRole).toString();
        ++row;
        //qDebug() << __FILE__ << QString::number(__LINE__) << " rows =" << QString::number(row) ;
        }
    ui->tableViewOfValues->setModel(model);
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
}

bool findReceiptsValues::tableViewIsFull(QAbstractItemModel * model){
    bool ret = false;
    int count = model->rowCount();
    if (count > 255)
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "table view is full" ;
    	  ret = true;
        }
    return ret;
}

void findReceiptsValues::enableShowNextTable(){
    qDebug() << __FILE__ << QString::number(__LINE__) << " enableshownet "   ;
    ui->nextButton->show();
}

/*void findReceiptsValues::showNext(){
    QAbstractItemModel * abModel = ui->tableViewOfValues->model();
    int rows = abModel->rowCount();
    qDebug() << __FILE__ << QString::number(__LINE__) << " row =" << QString::number(rows) ;
    int numberOfLastRow = abModel->headerData(rows -1,Qt::Vertical,Qt::DisplayRole).toInt();
    QString lastData = abModel->data(abModel->index(numberOfLastRow -1,0 ),Qt::DisplayRole).toString();
    qDebug() << __FILE__ << QString::number(__LINE__) << " numberOfLastRow =" << QString::number(numberOfLastRow) ;
    qDebug() << __FILE__ << QString::number(__LINE__) << " shownext data =" <<  lastData;
    QString comboChoice = ui->comboBoxCategories->currentText();
    QString afterSqlFilter = QString("%1 LIKE '%2' AND %3 >= '%4'").arg("TYPE",comboChoice,"NAME",lastData);
    model->setFilter(afterSqlFilter);
    int count =   model->rowCountWithFilter(QModelIndex(),afterSqlFilter);
    for (int i = 0; i < count; i += 1)
    {
    	QString name = model->dataWithFilter(model->index(i,MP_NAME),Qt::DisplayRole,afterSqlFilter).toString();
    	//qDebug() << __FILE__ << QString::number(__LINE__) << " names =" << name ;
    	QString value = model->dataWithFilter(model->index(i,MP_AMOUNT),Qt::DisplayRole,afterSqlFilter).toString();
    	QStandardItem *itemName = new QStandardItem(name);
    	QStandardItem *itemValue = new QStandardItem(value);
    	QList<QStandardItem*> list;
    	list << itemName << itemValue;
    	itemModel->appendRow(list);
        }
    model->setFilter("");
    QVariant act = QVariant(trUtf8("Name"));
    QVariant value = QVariant(trUtf8("Value"));
    if (!itemModel->setHeaderData(0,Qt::Horizontal,act,Qt::EditRole))
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
    	  }
    if (!itemModel->setHeaderData(1,Qt::Horizontal,value,Qt::EditRole)	)
    {
    	  qWarning() << __FILE__ << QString::number(__LINE__) << "no header data available";
        } 
    ui->tableViewOfValues->setModel(itemModel);
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
}*/

void findReceiptsValues::showNext(){
    QAbstractItemModel * abModel = ui->tableViewOfValues->model();
    int rows = abModel->rowCount();
    qDebug() << __FILE__ << QString::number(__LINE__) << " row =" << QString::number(rows) ;
    int numberOfLastRow = abModel->headerData(rows -1,Qt::Vertical,Qt::DisplayRole).toInt();
    QString lastData = abModel->data(abModel->index(numberOfLastRow -1,0 ),Qt::DisplayRole).toString();
    QString comboChoice = ui->comboBoxCategories->currentText();
    QString afterSqlFilter = QString("%1 LIKE '%2' AND %3 >= '%4'").arg("TYPE",comboChoice,"NAME",lastData);
    MedicalProcedureModel *model = new MedicalProcedureModel(this);
    model->setFilter(afterSqlFilter);
    ui->tableViewOfValues->setModel(model);
    ui->tableViewOfValues->setColumnHidden(MP_ID,true);
    ui->tableViewOfValues->setColumnHidden(MP_UID,true);
    ui->tableViewOfValues->setColumnHidden(MP_USER_UID,true);
    ui->tableViewOfValues->setColumnHidden(MP_INSURANCE_UID,true);
    ui->tableViewOfValues->setColumnHidden(MP_REIMBOURSEMENT,true);
    ui->tableViewOfValues->setColumnHidden(MP_ABSTRACT,true);
    ui->tableViewOfValues->setColumnHidden(MP_TYPE,true);
    ui->tableViewOfValues->setColumnHidden(MP_DATE,true);    
    ui->tableViewOfValues-> setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewOfValues-> setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewOfValues->horizontalHeader()->setStretchLastSection ( true );
    ui->tableViewOfValues->setGridStyle(Qt::NoPen);
}
