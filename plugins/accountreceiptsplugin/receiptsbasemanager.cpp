#include "receiptsbasemanager.h"
#include "xmlcategoriesparser.h"
#include <QMessageBox>
static  QString freeaccount = "freeaccount";

receiptsBaseManager::receiptsBaseManager()
{
}

receiptsBaseManager::~receiptsBaseManager()
{
}

QList<QMultiHash<int,QString> > receiptsBaseManager::getPercentages(){
  QList<QMultiHash<int,QString> > rList;
  QMultiHash<int,QString> type,percentage;
  QStringList idList;
    idList << "0" << "1" << "2" << "3";
  QStringList typeList;
    typeList << "zero" << "one third" << "two third"  << "Complete"   ;
  QStringList percentageList; 
    percentageList << "0" << "30" << "70" << "100"   ;
  QString strId;
  foreach(strId,idList){
     int key = strId.toInt();
     type.insert(key,typeList[key]);
     percentage.insert(key,percentageList[key]);
     }
  rList << type << percentage;
  return rList;
}

QStringList receiptsBaseManager::getComboBoxesDatas(const QString &values,const QString &table){
  QStringList list;
  QSqlDatabase db = QSqlDatabase::database(freeaccount);
  QSqlQuery query(db);
    QString req = QString("SELECT %1 FROM %2").arg(values,table);
    if(!query.exec(req)){
        qWarning()  << __FILE__ << QString::number(__LINE__) << query.lastError().text();
        }
    while(query.next()){
        QString str = query.value(0).toString();
        list << str;    
        }
  return list;
}

QString receiptsBaseManager::createTablesAndFields(){
    QString result = "Ok";
    qDebug() <<  __FILE__ << QString::number(__LINE__) ;
    QSqlDatabase db = QSqlDatabase::database("freeaccount");
    QString reqMP = QString("create table if not exists %1 (%2) ")
                                          .arg("medical_procedure",
                                               "MP_ID INTEGER PRIMARY KEY,"
                                               "MP_UUID varchar(50),"
                                               "MP_USER_UID varchar(50),"
                                               "NAME varchar(50),"
                                               "ABSTRACT varchar(100),"
                                               "TYPE varchar(50),"
                                               "AMOUNT real,"
                                               "REIMBOURSEMENT TEXT,"
                                               "DATE DATETIME");
    QString reqAccount = QString("create table if not exists %1 (%2)") //honoraires
                            .arg("account",
                                 "ACCOUNT_ID INTEGER PRIMARY KEY,"
                                 "ACCOUNT_UID varchar(50),"
                                 "USER_UID varchar(50),"
                                 "PATIENT_UID BIGINT,"
                                 "PATIENT_NAME VARCHAR(100),"
                                 "SITE_ID BIGINT,"
                                 "INSURANCE_ID BIGINT,"
                                 "DATE DATETIME ,"
                                 "MP_XML BLOB,"
                                 "MP_TXT LONGTEXT,"
                                 "COMMENT LONGTEXT,"
                                 "CASH REAL,"
                                 "CHEQUE REAL,"
                                 "VISA REAL,"
                                 "INSURANCE REAL,"
                                 "OTHER REAL,"
                                 "DUE REAL,"
                                 "DUE_BY REAL,"
                                 "ISVALID BOOL,"
                                 "TRACE BLOB");
    QString reqSites = QString("create table if not exists %1 (%2)")
                       .arg("sites",
                            "SITE_ID INTEGER PRIMARY KEY,"
                            "SITE_UID BIGINT,"
                            "NAME TEXT,"
                            "ADRESS LONGTEXT,"
                            "CITY TEXT,"
                            "ZIPCODE TEXT,"
                            "COUNTRY TEXT,"
                            "TEL TEXT,"
                            "FAX TEXT,"
                            "MAIL TEXT,"
                            "CONTACT TEXT");
    QString reqWho = QString("create table if not exists %1 (%2)")
                        .arg("users",
                             "USER_ID INTEGER PRIMARY KEY,"
                             "USER_UID BIGINT,"
                             "LOG TEXT,"
                             "PASS TEXT,"
                             "EMR_USER_UID TEXT,"
                             "ID_TYPE_RULES INT");
    QString reqBankDetails = QString("create table if not exists %1 (%2)")
                        .arg("bank_details",
                             "BD_ID INTEGER PRIMARY KEY,"
                             "BD_USER_UID TEXT,"
                             "BD_LABEL TEXT,"
                             "BD_OWNER TEXT,"
                             "BD_OWNERADRESS TEXT,"
                             "BD_ACCNUMB TEXT,"
                             "BD_IBAN TEXT,"
                             "BD_BALANCE REAL,"
                             "BD_BALANCEDATE DATETIME,"
                             "BD_COMMENT LONGTEXT,"
                             "BD_ISDEFAULT BOOL");
    
    QString reqRules = QString("create table if not exists %1 (%2)")
                          .arg("rules",
                               "ID_RULES INTEGER PRIMARY KEY,"
                               "ID_TYPE_RULES INT,"
                               "NAME_OF_RULE TEXT,"
                               "TYPE TEXT");
    QString reqDistanceRules = QString("create table if not exists %1 (%2)")
                                  .arg("distance_rules",
                                       "ID_DISTANCE_RULE INTEGER PRIMARY KEY,"
                                       "NAME_DIST_RULE TEXT,"
                                       "TYPE_DIST_RULE TEXT");
                                       
    QString reqDebtor = QString("create table if not exists %1 (%2)")
                           .arg("insurance",
                                "INSURANCE_ID INTEGER PRIMARY KEY,"
                                "INSURANCE_UID BIGINT,"
                                "NAME TEXT,"
                                "ADRESS LONGTEXT,"
                                "CITY TEXT,"
                                "ZIPCODE TEXT,"
                                "COUNTRY TEXT,"
                                "TEL TEXT,"
                                "FAX TEXT,"
                                "MAIL TEXT,"
                                "CONTACT TEXT");
                          

    QStringList listOfReq;
    listOfReq << reqMP 
              << reqAccount << reqSites << reqWho << reqBankDetails << reqRules << reqDistanceRules << reqDebtor ;
    QString strReq;
    foreach(strReq,listOfReq){
        QSqlQuery q(db);
        if(!q.exec(strReq)){
            qWarning() << __FILE__ << QString::number(__LINE__) << q.lastError().text() ;
            result = q.lastError().text() ;
            
            }
    
         }                                              
        qDebug() <<  __FILE__ << QString::number(__LINE__) ;
        return result;
}

bool receiptsBaseManager::writeAllDefaultsValues(){
  bool ret = true;
  QSqlDatabase db = QSqlDatabase::database(freeaccount);
  xmlCategoriesParser xml;
  QStringList listOfReq;
  QList<QHash<QString,QString> > hashList;
  hashList = xml.readXmlFile();
  QHash<QString,QString> hash = hashList[0];
  QStringList nameOfActsList = hash.keys();
  nameOfActsList.removeAll("typesOfReceipts");
  QString nameOfKeys = nameOfActsList.join(",");
  qDebug() << __FILE__ << QString::number(__LINE__) << nameOfKeys;
  
  QString strAct;
  foreach(strAct,nameOfActsList){
      QHash<QString,QString> hashValues;
      QString xmlValue = hash.value(strAct);
      qDebug() << __FILE__ << QString::number(__LINE__) << "strAct ="+strAct+" values = "+xmlValue;
      QStringList valuesList = xmlValue.split(",");
      QString strValue;
      foreach(strValue,valuesList){
            if(strValue.contains("=")){
                QStringList pair = strValue.replace(" ","").split("=");
                qDebug() << __FILE__ << QString::number(__LINE__) << pair[0] << " "<<pair[1];
                hashValues.insert(pair[0],pair[1]);
            }
            
            }
            qDebug() << __FILE__ << QString::number(__LINE__) << " TYPE = "+hashValues.value("TYPE");
      listOfReq << QString("INSERT INTO %1 (%2) VALUES(%3)")
                          .arg("medical_procedure",
                               "NAME,ABSTRACT,TYPE,AMOUNT,REIMBOURSEMENT,DATE",
                               "'"+strAct+"',"
                               "'"+hashValues.value("ABSTRACT")+"',"
                               "'"+hashValues.value("TYPE")+"',"
                               "'"+hashValues.value("AMOUNT")+"',"
                               "'"+hashValues.value("REIMBOURSEMENT")+"',"
                               "'"+hashValues.value("DATE")+"'");
      }
   QSqlQuery q(db);
   QString req;
   foreach(req,listOfReq){
       qDebug() << __FILE__ << QString::number(__LINE__) << "requetes = "+req;
       m_rbmReq += req;
       if(!q.exec(req)){
           qWarning()  << __FILE__ << QString::number(__LINE__) << q.lastError().text();
           ret = false;
           }
       }
   return ret;
}

QStringList receiptsBaseManager::getChoiceFromCategories(QString & categoriesItem){
    QStringList listOfItems;
    QSqlDatabase db = QSqlDatabase::database(freeaccount);
    QString item = categoriesItem;
    qDebug()  << __FILE__ << QString::number(__LINE__) << " categories item ="+item;
    if(item == "thesaurus"){QMessageBox::information(0,"Info","item = "+item,QMessageBox::Ok);}
    else if(item == "CCAM"){QMessageBox::information(0,"Info","show CCAM widget",QMessageBox::Ok);}
    else{
        QString req = QString("SELECT %1 FROM %2 WHERE %3 = '%4'").arg("NAME","medical_procedure","TYPE",item);
        QSqlQuery q(db);
        if(!q.exec(req)){
           qWarning()  << __FILE__ << QString::number(__LINE__) << q.lastError().text();
           listOfItems << trUtf8("Error");
           }
        while(q.next()){
            QString name = q.value(0).toString();
            qDebug()  << __FILE__ << QString::number(__LINE__) << " choice item ="+name;
            listOfItems << name;
            }
       }
    return listOfItems;
}


