
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <list>
#include <sstream>
#include <fstream>
#include <string>

#include "TList.h"
#include "string.h"
#include "TSystem.h"

#include "FairDbConnection.h"
#include "FairDbStatement.h"

#include <iostream>
using namespace std;


ClassImp(FairDbConnection)

FairDbConnection::FairDbConnection(
  const string& url,
  const string& user,
  const string& password)
  : fDbName(),
    fUser(user),
    fPassword(password),
    fExistingTableList(),
    fNumConnectedStatements(0),
    fUrl(url.c_str()),
    fUrlValidated(kFALSE),
    fIsTemporary(kTRUE),
    fDbType(FairDb::kUnknownDbType),
    fServer(NULL),
    fExceptionLog()
{
// Wrapper to a DB connection
  cout << "-I- FairDbConnection : Creating a DB connection" << endl;

  if ( this->Open() ) {
    cout << "-I- FairDbConnection: successfully opened connection to: "
         << this->GetUrl() << endl;
    fUrlValidated =  kTRUE;


    // First get which server do we use ...
    string productName(fServer->GetDBMS());
    // Get the c_strbase type
    if( productName == "MySQL" ) { fDbType = FairDb::kMySQL; }
    else if ( productName == "Oracle" ) { fDbType = FairDb::kOracle; }
    else if ( productName == "PgSQL" ) { fDbType = FairDb::kPostgreSQL; }

    else {
      cout<< "-E- FairDbConnection : Cannot determine DB type from name: "
          <<  productName
          << "\nWill assuming Oracle database " << endl;
      fDbType = FairDb::kOracle;
    }


    // Initialise the list existing supported tables.
    // Here all existing tables from the database will be added to the
    // central list of existing tables;

    this->SetTableExists();

    // Check if one can use prepared statement on the server
    if ( fUrlValidated ) {
      if ( ! fServer->HasStatement() ) {
        cout << "-I- FairDbConnection : This client does not support prepared statements." << endl;
        fUrlValidated = kFALSE;
      }
      string serverInfo(fServer->ServerInfo());
      if ( fDbType == FairDb::kMySQL &&  serverInfo < "4.1" ) {
        cout << "-I- FairDbConnection: this MySQL server (" << serverInfo
             << ") does not support prepared statements." << endl;
        fUrlValidated = kFALSE;
      }
      if ( fUrlValidated ) {
        cout << "-I- FairDbConnection: this client, and MySQL server ("
             << serverInfo
             << ") does support prepared statements." << endl;
      } else {

        cout<< "-I- FairDbConnection "<< endl;
        cout<< "This version of MySQL does not support prepared statements.\n"
            << "\n"
            << "Please upgrade to MySQL (client and server) version 4.1 or greater \n"
            << "\n"
            << endl;
      }

    }//! fUrlValidated

    if ( ! fUrlValidated ) {
      cout << "-E- FairDbConnection : Aborting due to above errors" << endl;
      exit(1);
    }
  }

  fDbName = fUrl.GetFile();
}


FairDbConnection::~FairDbConnection()
{

  cout <<"-I- FairDBConnection::~FairDbConnection() : Droping Connection"
       <<endl;
  this->Close(kTRUE);

}

Bool_t FairDbConnection::Open()
{

  if ( !this->IsClosed() ) { return kTRUE;}

  Int_t maxAttempt = fUrlValidated ? 100 : 20;
  for (Int_t attempt = 1; attempt <= maxAttempt; ++attempt) {
    // Main Connection call
    fServer = TSQLServer::Connect(
                fUrl.GetUrl(),
                fUser.c_str(),
                fPassword.c_str());
    if ( ! fServer ) {
      cout << "-E- Failing to open: " <<
           fUrl.GetUrl() << " for user "
           << fUser << " and password "
           << fPassword << " (attempt "
           << attempt << ")";

      if ( attempt == 1 ) {
        cout <<"-I- FairDBConnection: Retrying ... " << endl;
      }
      gSystem->Sleep(attempt*1000);
    } else {
      fServer->EnableErrorOutput(kTRUE);
      if ( attempt > 1 ) {
        cout << "-I- FairDbConnection Connection opened on attempt "
             << attempt << endl;
      }
      cout << "-I- Successfully opened connection to: "
           << fUrl.GetUrl() << endl;

      // <DB> Check me about !!!
      // ASCII database, populate it and make the connection permanent
      // unless even ASCII DB connections are temporary.
      return kTRUE;
    }
  }//! for on attempt

  cout  <<  " -E- FairDbConnection: Failed to open a connection to: "
        << fUrl.GetUrl()
        << " for user " << fUser << " and pwd " << fPassword << endl;

  return kFALSE;
}

Bool_t FairDbConnection::Close(Bool_t force)
{

  if ( this->IsClosed() ) { return kTRUE; }

  if ( fNumConnectedStatements ) {
    if ( ! force ) {
      cout<< "-I- FairDbConnection Unable to close connection: "
          << this->GetUrl()
          << "; it still has  "
          << fNumConnectedStatements << "active statements. " << endl;
      return kFALSE;
    }
    cout << "-I- FairDbConnection : closing connection: "
         << this->GetUrl()
         << "; even though it still has "
         << fNumConnectedStatements << " active statements. " << endl;
  }

  delete fServer;
  fServer = NULL;
  cout << "-I- FairDbConnection: closed connection: "
       << this->GetUrl() << endl;
  return kTRUE;
}

void FairDbConnection::CloseIdleConnection()
{
  if ( fIsTemporary &&  fNumConnectedStatements == 0 ) { this->Close(); }
}


//.....................................................................

TSQLStatement* FairDbConnection::CreatePreparedStatement(const string& sql)
{

  TSQLStatement* stmt = 0;
  if ( ! this->Open() ) { return stmt; }
  stmt = fServer->Statement(sql.c_str());
  if ( ! stmt ) {
    cout<< "-I- FairDbConnection::CreatePreparedStatement "
        << " no Statement created " << endl;
  } else { stmt->EnableErrorOutput(kFALSE); }

  return stmt;
}
//.....................................................................

TSQLServer* FairDbConnection::GetServer()
{
  if ( ! this->Open() ) { return 0; }
  return fServer;
}

const string& FairDbConnection::GetUrl() const
{
  static string url;
  url = const_cast<FairDbConnection*>(this)->fUrl.GetUrl();
  return url;

}

void  FairDbConnection::SetTableExists(const string& tableName)
{

  // cout << "-I- FairDbConnection::SetTableExists()  ... " <<  tableName << endl;

  // Add name to list of existing tables
  // (necessary when creating tables)
  //  Note: If tableName is null refresh list from the database.

  if ( tableName == "" ) {
    TSQLStatement* stmt;
    if ( fDbType == FairDb::kMySQL || fDbType == FairDb::kOracle ) {
      // MYSQL or Oracle
      stmt =  CreatePreparedStatement("show tables");
    } else {
      // POSTGRES
      stmt =  CreatePreparedStatement("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public'");
    }

    if ( stmt ) {
      if (stmt->Process()) {
        stmt->StoreResult();
        while (stmt->NextResultRow()) {
          string tn(stmt->GetString(0));
          std::transform(tn.begin(), tn.end(), tn.begin(), ::toupper);
          this->SetTableExists(tn);
        }
      }
      delete stmt;
      stmt = 0;
    }
  } else {
    if ( ! this->TableExists(tableName) ) {
      fExistingTableList += ",'";
      fExistingTableList += tableName;
      fExistingTableList += "'";
      // cout << "-I- FairDbConnection::Table Registered  ... " <<  tableName << endl;
    }
  }
  //cout << "-I- FairDbConnection:: End of table exists ... " << endl;
}

//.....................................................................

Bool_t  FairDbConnection::TableExists(const string& tableName) const
{
//
//  Check to see table exists in connected database.

  string test("'");
  test += tableName;
  test += "'";
  Bool_t btest =  fExistingTableList.find(test) != std::string::npos;
  //cout << " Table exists " << btest << endl;
  return btest;
}




Bool_t FairDbConnection::PrintExceptionLog(Int_t level) const
{
  return fExceptionLog.Size() != 0;
}

void  FairDbConnection::RecordException()
{
  fExceptionLog.AddEntry(*fServer);
}



