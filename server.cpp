#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdbool>
#include <climits>
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <cstring>
#include <set>
#include <mutex>
#include <vector>
#include <utility>

using namespace std;

#define SERVERPORT 2908

mutex log;
set<string> usernames;

static int callback(void*, int, char**, char**);
int login(int sd, char* comanda, int logged, int othCl);
int logout(int sd, int logged);
void clientTh(int sd);
void registr(int sd, char* comanda, int logged);
void wrong(int sd);
void checkDB(int sd, int logged, char* comanda, char* username);
void post(int sd, int logged, char* username, char* comanda);
int checkLoginOtherCl(set<string> usernames, char* username);
void viewPosts(int sd, int logged, char* username);
void addFriend(int sd, int logged, char* username, char* comanda);
int checkUsername(char* username);
int searchReq(int sd, int logged, char* username);
int nrRowsInTable(char* name);
void answerReq(int sd, char* username);
void chat(int sd, int logged, char* username, char* comanda);
void msg(int sd, int logged, char* username, char* comanda);
int checkAdmin(char* username);
int countFriends(char* username);
int checkFriends(char* username, char* comanda);
int checkReq(char* username, char* comanda);
void writeToClient(int sd, const char* mesaj);
void showCommands(int sd);

int main(int argc, char *argv[])
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;         //socket descriptor

    //creez socket
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket()\n");
    }
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVERPORT);

    //atasez socketul
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
        perror("Eroare la bind()\n");
        return 1;
    }

    //ascult daca vin clienti
    if (listen (sd, 100) == -1)
    {
        perror("Eroare la listen()\n");
    }
    while (1)
    {
        printf("Astept conexiuni...\n");

        int client;
        unsigned int length = sizeof(from);

        if ( (client = accept(sd, (struct sockaddr *) &from, &length)) < 0)
        {
            perror("Eroare la accept()\n");
            continue;
        }

//        td = (struct thData*)malloc(sizeof(struct thData));
//        td->idThread = i++;
//        td->cl = client;

//        thread thCl(clientTh, td);
        thread thCl(clientTh, client);
        thCl.detach();
    }
}

void clientTh(int sd)
{
    fflush (stdout);

    int logged = 0;
    int othCl = 0;
    int lgComanda;
    char* comanda;
    char* username;
    while (1)
    {
        if (read(sd, &lgComanda, sizeof(int)) == 0)
            break;
        comanda = (char*)malloc(lgComanda + 1);
        read(sd, comanda, lgComanda);
        comanda[lgComanda] = '\0';

        if (strncmp(comanda, "login", strlen("login")) == 0)
        {
            char* cComanda;
            cComanda = (char*)malloc(lgComanda);
            strcpy(cComanda, comanda);
            char* apel = strtok(cComanda, " \n");
            username = strtok(NULL, " \n");

            othCl = checkLoginOtherCl(usernames, username);
//            if (othCl == 0)
//            {
                printf("Comanda este login\n");
                logged = login(sd, comanda, logged, othCl);
            //}
            if (logged == 1)
            {
                usernames.insert((string)username);
            }
        }
        else if (strncmp(comanda, "register", strlen("register")) == 0)
        {
            printf("Comanda este register\n");
            registr(sd, comanda, logged);
        }
        else if (strncmp(comanda, "check", strlen("check")) == 0)
        {
            printf("Comanda este check\n");
            checkDB(sd, logged, comanda, username);
        }
        else if (strncmp(comanda, "logout", strlen("logout")) == 0)
        {
            if (logged == 1)
                usernames.erase((string)username);
            printf("Comanda este logout\n");
            logged = logout(sd, logged);
        }
        else if (strncmp(comanda, "post", strlen("post")) == 0)
        {
            printf("Comanda este post\n");
            post(sd, logged, username, comanda);
        }
        else if(strncmp(comanda, "viewposts", strlen("viewposts")) == 0)
        {
            printf("Comanda este viewposts\n");
            viewPosts(sd, logged, username);
        }
        else if (strncmp(comanda, "add_friend", strlen("add_friend")) == 0)
        {
            printf("Comanda este add_friend\n");
            //checkFriends(username, comanda);
            addFriend(sd, logged, username, comanda);
        }
        else if (strncmp(comanda, "chat", strlen("chat")) == 0)
        {
            printf("Comanda este chat\n");
            chat(sd, logged, username, comanda);
        }
        else if (strncmp(comanda, "msg", strlen("msg")) == 0)
        {
            printf("Comanda este msg\n");
            msg(sd, logged, username, comanda);
        }
        else if (strncmp(comanda, "show_commands", strlen("show_commands")) == 0)
        {
            printf("Comanda este show_commands\n");
            showCommands(sd);
        }
        else
        {
            printf("Comanda gresita!\n");
            wrong(sd);
        }

    }
    usernames.erase(username);
    printf("Exited thread\n");
}

int login(int sd, char* comanda, int logged, int othCl)
{
    printf("Sunt logat? %d\n", logged);
    if (logged == 1)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));

        const char* mesaj = "You are already logged in!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
        return logged;
    }
    else if (logged == 0 && othCl == 0)
    {
        int lgMesaj;

        char *username, *password, *apel;
        apel = strtok(comanda, " \n");
        username = strtok(NULL, " \n");
        password = strtok(NULL, " \n");

        sqlite3 *db;
        char *zErrMsg = 0;
        int rc;
        const char* data = "Callback function called";
        sqlite3_stmt* stmt = NULL;

        /* Open database */
        rc = sqlite3_open("users.db", &db);

        if( rc )
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "Opened database successfully\n");

        string sql = "SELECT COUNT(*) FROM USERS WHERE USERNAME LIKE '" + (string)username + "' AND PASSWORD LIKE '"
                +(string)password + "';";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        printf("Apel SQL: [%s]\n", sql.c_str());
        printf("Comanda: [%s]\n", apel);
        printf("Username: [%s]\n", username);
        printf("Password: [%s]\n", password);

        int step = sqlite3_step(stmt);

        if (step != SQLITE_ROW)
        {
            cout << "step gresit";
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        logged = sqlite3_column_int (stmt, 0);

        printf("rezultatul este %d\n", logged);

        int final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";


        sqlite3_close(db);

        if (logged == 1)
        {
            searchReq(sd, logged, username);
            answerReq(sd, username);
        }
        else
        {
            int nrMsg = 1;
            write(sd, &nrMsg, sizeof(int));
            const char* mesaj = "Username or password not found!\n";
            lgMesaj = strlen(mesaj);
            write(sd, &lgMesaj, sizeof(int));
            write(sd, mesaj, lgMesaj);
        }
    }
    else if (logged == 0 && othCl == 1)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are already logged in from another client!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
    }
    return logged;
}

void registr(int sd, char* comanda, int logged)
{
    int nrMsg = 1;

    if (logged == 1)
    {
        write(sd, &nrMsg, sizeof(int));

        const char* mesaj = "You are already logged in!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
        return;
    }

    write(sd, &nrMsg, sizeof(int));

    char* username, *password, *apel, *admin;

    apel = strtok(comanda, " \n");
    username = strtok(NULL, " \n");
    password = strtok(NULL, " \n");
    admin = strtok(NULL, " \n");
    char atmin;
    if (strcmp(admin, "admin") == 0)
        atmin = '1';
    else
        atmin = '0';

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM USERS WHERE USERNAME LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
        cout << "step gresit";
    else
        fprintf(stdout, "Operation done successfully\n");

    int userFound = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";

    printf("rezultatul este %d\n", userFound);

    if (userFound == 0)
    {
        char log = '0';
        string sql_2 = "INSERT INTO USERS (USERNAME,PASSWORD,LOGGED,ADMIN) VALUES ('" + (string)username + "','" +
                (string)password + "'," + log + "," + atmin + ");";
        char* aux;
        rc = sqlite3_exec(db, sql_2.c_str(), nullptr, 0, &aux);
        cout << aux << '\n';

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }
        printf("Comanda SQL: [%s]\n", sql_2.c_str());
        printf("Username: [%s]\n", username);
        printf("Password: [%s]\n", password);

        const char* mesaj = "Account created successfully!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
    }
    else
    {
        const char* mesaj = "Username already in use!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
    }

    sqlite3_close(db);
}

void checkDB(int sd, int logged, char* comanda, char* username)
{
    if (logged == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

    if (checkAdmin(username) == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You do not have access to this command!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    //const char *sql;
    const char* data = "";
    sqlite3_stmt* stmt;
    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    char* apel = strtok(comanda, " \n");
    char* tabel = strtok(NULL, " \n");



        /* Create SQL statement */
        string sql = "SELECT name FROM pragma_table_info('" + (string)tabel + "') ORDER BY cid;";

//        string sql = "DELETE FROM MESSAGES WHERE USERNAME1 LIKE 'mihai' AND USERNAME2 LIKE 'calin'";
//    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

    /* Execute SQL statement */
    int nrMsg = nrRowsInTable(tabel);
    //write(sd, &nrMsg, sizeof(int));
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }


    vector<string> colNames;
    int step = sqlite3_step(stmt);
    while (step == SQLITE_ROW)
    {
        char* nume = (char*)sqlite3_column_text(stmt, 0);
        colNames.push_back((string)nume);
        step = sqlite3_step(stmt);
    }
    for (auto it : colNames)
        cout << it << '\n';

    int mesaje = (colNames.size() + 1) * nrMsg;
    write(sd, &mesaje, sizeof(int));

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sql = "SELECT * FROM " + (string)tabel;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    step = sqlite3_step(stmt);
    //int i = 0;
    while (step == SQLITE_ROW)
    {
        for (int j = 0; j < colNames.size(); j++)
        {
            char* text = (char*)sqlite3_column_text(stmt, j);
            string mesaj = colNames[j] + " = " + (string)text;
            int lgMsg = mesaj.length();
            write(sd, &lgMsg, sizeof(int));
            write(sd, mesaj.c_str(), lgMsg);
        }
        string pauza = "==================";
        int lgPauza = pauza.length();
        write(sd, &lgPauza, sizeof(int));
        write(sd, pauza.c_str(), lgPauza);
//        i++;
//        if (i == colNames.size())
//            i = 0;
        step = sqlite3_step(stmt);
    }
    final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);



//        const char* mesaj = "Check command called\n";
//        int lgMesaj = strlen(mesaj);
//        write(sd, &lgMesaj, sizeof(int));
//        write(sd, mesaj, lgMesaj);
}

static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char*)data);

    for(i = 0; i<argc; i++)
    {
//        if (argv[i])
//        {
//            int lg = strlen(argv[i]);
//            write(sd, &lg, sizeof(int));
//            write(sd, argv, lg);
//        }
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

void wrong(int sd)
{
    int nrMsg = 1;
    write(sd, &nrMsg, sizeof(int));

    const char* mesaj = "Command not recognized!\n";
    int lgMesaj = strlen(mesaj);
    write(sd, &lgMesaj, sizeof(int));
    write(sd, mesaj, lgMesaj);
}

int logout(int sd, int logged)
{
    int nrMsg = 1;

    if (logged == 0)
    {
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
        return logged;
    }

    write(sd, &nrMsg, sizeof(int));

    const char* mesaj = "You have logged out!\n";
    int lgMesaj = strlen(mesaj);
    write(sd, &lgMesaj, sizeof(int));
    write(sd, mesaj, lgMesaj);
    return 0;
}

void post(int sd, int logged, char* username, char* comanda)
{
    if (logged == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
        return;
    }
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    const char* apel, *preference;
    char *text;

    rc = sqlite3_open("users.db", &db);
    if (rc)
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    printf("Comanda este: %s\n", comanda);

    int lgComanda = strlen(comanda);
    apel = strtok(comanda, " \n");
    preference = strtok(NULL, " \n");

    int lgApel = strlen(apel);
    int lgPref = strlen(preference);

    text = (char*)malloc(lgComanda - lgApel - lgPref - 2);
    strcpy(text, comanda + lgApel + lgPref + 2);

    char publ = '0';

    printf("Username este: %s\n", username);
    if (strcmp(preference, "public") == 0)
        publ = '1';

    printf("Username este: %s\n", username);
    printf("Pref este: %s\n", preference);
    printf("Text este: %s\n", text);
    if (text[strlen(text) - 1] == '\n')
        text[strlen(text) - 1] = '\0';

    string sql = "INSERT INTO POSTS (USERNAME,TEXT,PUBLIC) VALUES ('"
            + (string)username + "','" + (string)text + "'," + publ + ");";

    printf("Apel SQL este: [%s]\n", sql.c_str());

    char* aux = nullptr;
    rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &aux);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", aux);
        sqlite3_free(zErrMsg);
    }
    else
        fprintf(stdout, "Operation done succesfully!\n");

    int nrMsg = 1;
    write(sd, &nrMsg, sizeof(int));

    const char* mesaj = "Post created successfully!\n";
    int lgMesaj = strlen(mesaj);
    write(sd, &lgMesaj, sizeof(int));
    write(sd, mesaj, lgMesaj);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
}

int checkLoginOtherCl(set<string> usernames, char* username)
{
    log.lock();
    int logged = 0;
    if (usernames.empty())
        logged = 0;
    else if (usernames.find(username) != usernames.end())
        logged = 1;
    log.unlock();
    return logged;
}

void viewPosts(int sd, int logged, char* username)
{
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    rc = sqlite3_open("users.db", &db);
    if (rc)
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql_usr, sql_txt;
    if (logged == 1)
    {
        const char* sql_count_public = "SELECT COUNT(*) FROM POSTS WHERE PUBLIC = 1;";
        rc = sqlite3_prepare_v2(db, sql_count_public, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
            cout << "primul rc gresit\n";
        int step = sqlite3_step(stmt);
        if (step != SQLITE_ROW)
            cout << "primul step gresit\n";
        int result = sqlite3_column_int(stmt, 0);
        int nrMsg = result;
        int final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";

        vector<string> friends;
        if (countFriends(username) != 0)
        {
            string sql_f = "SELECT USERNAME1, USERNAME2 FROM FRIENDS WHERE USERNAME1 LIKE '" +
                           (string) username + "' OR USERNAME2 LIKE '" + (string) username + "';";
            rc = sqlite3_prepare_v2(db, sql_f.c_str(), -1, &stmt, NULL);
            if (rc != SQLITE_OK)
                cout << "primul rc gresit\n";
            step = sqlite3_step(stmt);
            if (step != SQLITE_ROW)
                cout << "primul step gresit\n";

            while (step == SQLITE_ROW)
            {
                char *nume = (char *) sqlite3_column_text(stmt, 0);
                if (strcmp(nume, username) == 0)
                {
                    char *numeBun = (char *) sqlite3_column_text(stmt, 1);
                    friends.push_back((string) numeBun);
                } else
                    friends.push_back((string) (nume));
                step = sqlite3_step(stmt);
            }

            final = sqlite3_finalize(stmt);
            if (final != SQLITE_OK)
                cout << "final_gresit\n";

            for (auto it: friends)
            {
                string sql_2 = "SELECT COUNT(*) FROM POSTS WHERE USERNAME LIKE '" + it + "' AND PUBLIC = 0;";
                sqlite3_prepare_v2(db, sql_2.c_str(), -1, &stmt, NULL);
                sqlite3_step(stmt);
                int nr = sqlite3_column_int(stmt, 0);
                nrMsg += nr;
            }
            final = sqlite3_finalize(stmt);
            if (final != SQLITE_OK)
                cout << "final_gresit\n";
        }

        write(sd, &nrMsg, sizeof(int));



        sql_usr = "SELECT USERNAME, TEXT, PUBLIC FROM POSTS WHERE PUBLIC = 1";
        rc = sqlite3_prepare_v2(db, sql_usr.c_str(), -1, &stmt, NULL);

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        step = sqlite3_step(stmt);

        if (step != SQLITE_ROW)// && step2 != SQLITE_ROW)
        {
            cout << "step gresit";
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        int it = 0;
        char* text[1000];
        char* username1[1000];
        int pbl[1000];
        char pref[10];
        while (step == SQLITE_ROW)
        {
            username1[it] = (char *) sqlite3_column_text(stmt, 0);
            text[it] = (char*) sqlite3_column_text(stmt, 1);
            pbl[it] = sqlite3_column_int(stmt, 2);
            if (pbl[it] == 1)
                strcpy(pref, "public");
            else
                strcpy(pref, "friends");

            string mesaj = (string)username1[it] + " posted (" + (string)pref + "): " + (string)text[it];
            int lgMesaj = mesaj.length();
            write(sd, &lgMesaj, sizeof(int));
            write(sd, mesaj.c_str(), lgMesaj);

            printf("%s\n", mesaj.c_str());
            it++;
            step = sqlite3_step(stmt);
        }

        //=================================
        if (countFriends(username) != 0)
        {
            for (auto it: friends)
            {
                string sql_usr_2 = "SELECT USERNAME, TEXT, PUBLIC FROM POSTS WHERE PUBLIC = 0 AND USERNAME LIKE '"
                                   + it + "';";
                rc = sqlite3_prepare_v2(db, sql_usr_2.c_str(), -1, &stmt, NULL);

                if (rc != SQLITE_OK)
                {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                } else
                {
                    fprintf(stdout, "Operation done successfully\n");
                }

                step = sqlite3_step(stmt);

                if (step != SQLITE_ROW)// && step2 != SQLITE_ROW)
                {
                    cout << "step gresit";
                } else
                {
                    fprintf(stdout, "Operation done successfully\n");
                }

                int it2 = 0;
                char *text2;
                char *username2;
                int pbl2;
                char pref2[10];
                while (step == SQLITE_ROW)
                {
                    username2 = (char *) sqlite3_column_text(stmt, 0);
                    text2 = (char *) sqlite3_column_text(stmt, 1);
                    pbl2 = sqlite3_column_int(stmt, 2);
                    if (pbl2 == 1)
                        strcpy(pref2, "public");
                    else
                        strcpy(pref2, "friends");

                    string mesaj = (string) username2 + " posted (" + (string) pref2 + "): " + (string) text2;
                    int lgMesaj = mesaj.length();
                    write(sd, &lgMesaj, sizeof(int));
                    write(sd, mesaj.c_str(), lgMesaj);

                    printf("%s\n", mesaj.c_str());
                    step = sqlite3_step(stmt);
                }
                final = sqlite3_finalize(stmt);
                if (final != SQLITE_OK)
                    cout << "final_gresit\n";
            }
            cout << it << '\n';
        }
    }

    else if (logged == 0)
    {
        const char* sql_count = "SELECT COUNT(*) FROM POSTS WHERE PUBLIC = 1;";
        rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
            cout << "primul rc gresit\n";
        int step = sqlite3_step(stmt);
        if (step != SQLITE_ROW)
            cout << "primul step gresit\n";
        int result = sqlite3_column_int(stmt, 0);
        int nrMsg = result;
        nrMsg++;
        write(sd, &nrMsg, sizeof(int));

        int final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";

        sql_usr = "SELECT USERNAME, TEXT, PUBLIC FROM POSTS WHERE PUBLIC = 1";
        rc = sqlite3_prepare_v2(db, sql_usr.c_str(), -1, &stmt, NULL);

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        step = sqlite3_step(stmt);

        if (step != SQLITE_ROW)// && step2 != SQLITE_ROW)
        {
            cout << "step gresit";
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        int it = 0;
        char* text[1000];
        char* username[1000];
        int pbl[1000];
        char pref[10];
        while (step == SQLITE_ROW)
        {
            username[it] = (char *) sqlite3_column_text(stmt, 0);
            text[it] = (char*) sqlite3_column_text(stmt, 1);
            pbl[it] = sqlite3_column_int(stmt, 2);
            const char* pref = "public";

            string mesaj = (string)username[it] + " posted (" + (string)pref + "): " + (string)text[it];
            int lgMesaj = mesaj.length();
            write(sd, &lgMesaj, sizeof(int));
            write(sd, mesaj.c_str(), lgMesaj);

            printf("%s\n", mesaj.c_str());
            it++;
            step = sqlite3_step(stmt);
        }
        cout << it << '\n';
        final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";


        const char* mesaj = "You are not logged in. You can only view public posts.\n";
        int lgMesaj = strlen(mesaj);
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj, lgMesaj);
    }
    sqlite3_close(db);
}

void addFriend(int sd, int logged, char* username, char* comanda)
{
    int lgComanda = strlen(comanda);
    char* cCom = (char*)malloc(lgComanda);
    char* c2Com = (char*)malloc(lgComanda);
    strcpy(cCom, comanda);
    strcpy(c2Com, comanda);
    char* apel = strtok(comanda, " \n");
    char* dest = strtok(NULL, " \n");

    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    if (logged == 1)
    {
        if (strcmp(username, dest) == 0)
        {
            int nrMsg = 1;
            write(sd, &nrMsg, sizeof(int));
            const char* mesaj = "You can't add yourself as a friend!\n";
            int lgMsg = strlen(mesaj);
            write(sd, &lgMsg, sizeof(int));
            write(sd, mesaj, lgMsg);
            return;
        }

//        int frie = checkFriends(username, comanda);

        if (checkFriends(username, cCom) == 1)
        {
            int nrMsg = 1;
            write(sd, &nrMsg, sizeof(int));
            string mesaj = "You are already friends with " + (string)dest + "!\n";
            int lgMsg = mesaj.length();
            write(sd, &lgMsg, sizeof(int));
            write(sd, mesaj.c_str(), lgMsg);
            return;
        }

        if (checkReq(username, c2Com) == 1)
        {
            int nrMsg = 1;
            write(sd, &nrMsg, sizeof(int));
            string mesaj = "You have already sent a friend request to " + (string)dest + "!\n";
            int lgMsg = mesaj.length();
            write(sd, &lgMsg, sizeof(int));
            write(sd, mesaj.c_str(), lgMsg);
            return;
        }

        rc = sqlite3_open("users.db", &db);
        if (rc)
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "Opened database successfully\n");

        if (checkUsername(dest) == 0)
        {
            int nrMsg = 1;
            write(sd, &nrMsg, sizeof(int));
            const char* mesaj = "User does not exist!\n";
            int lgMsg = strlen(mesaj);
            write(sd, &lgMsg, sizeof(int));
            write(sd, mesaj, lgMsg);
            return;
        }

        string sql = "INSERT INTO FRND_REQUESTS (USERNAME1,USERNAME2) VALUES ('" + (string)username + "', '" +
                (string)dest + "');";

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }

        sqlite3_step(stmt);

        printf("%s\n", sql.c_str());

        int final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";

        sqlite3_close(db);

        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "Friend request sent.\n";
        int lgMsg = strlen(mesaj);
        write(sd, &lgMsg, sizeof(int));
        write(sd, mesaj, lgMsg);
    }
    else
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lgMsg = strlen(mesaj);
        write(sd, &lgMsg, sizeof(int));
        write(sd, mesaj, lgMsg);
    }
}

int checkUsername(char* username)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM USERS WHERE USERNAME LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int userFound = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    return userFound;
}

int searchReq(int sd, int logged, char* username)
{
    int nrReq;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM FRND_REQUESTS WHERE USERNAME2 LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    nrReq = sqlite3_column_int (stmt, 0);
    nrReq++;
    cout << nrReq << '\n';
    write(sd, &nrReq, sizeof(int));
    const char* logat = "Succesfully logged in!\n";
    int lgMesaj = strlen(logat);
    write(sd, &lgMesaj, sizeof(int));
    write(sd, logat, lgMesaj);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";

    sql = "SELECT USERNAME1 FROM FRND_REQUESTS WHERE USERNAME2 LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    step = sqlite3_step(stmt);

    while ( step == SQLITE_ROW )
    {
        char* src = (char*)sqlite3_column_text(stmt, 0);
        string mesaj = (string)src + " sent you a friend request! [accept/cancel username]";
        lgMesaj = mesaj.length();
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj.c_str(), lgMesaj);
        step = sqlite3_step(stmt);
        cout << "Am afisat pe ecran\n";
    }

    final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nrReq;
}

void answerReq(int sd, char* username)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    rc = sqlite3_open("users.db", &db);

    string sql = "SELECT USERNAME1 FROM FRND_REQUESTS WHERE USERNAME2 LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    int step = sqlite3_step(stmt);

    vector<pair<string, string>> acc, ref;
    while ( step == SQLITE_ROW )
    {
        int lgComanda;
        read(sd, &lgComanda, sizeof(int));
        char* comanda = (char*)malloc(lgComanda);
        read(sd, comanda, lgComanda);
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));

        char* option = strtok(comanda, " \n");
        char* user2 = strtok(NULL, " \n");

        if (strcmp(option, "accept") == 0)
        {
            cout << "Am acceptat\n";
            char* src = (char*)sqlite3_column_text(stmt, 0);
            string mesaj = "You are now friends with " + (string)src + "!";
            int lgMesaj = mesaj.length();
            write(sd, &lgMesaj, sizeof(int));
            write(sd, mesaj.c_str(), lgMesaj);

            pair<string, string> per((string)user2, (string)username);
            acc.push_back(per);
        }
        else if (strcmp(option, "cancel") == 0)
        {
            cout << "Am refuzat\n";
            char* src = (char*)sqlite3_column_text(stmt, 0);
            string mesaj = "Friend request rejected!";
            int lgMesaj = mesaj.length();
            write(sd, &lgMesaj, sizeof(int));
            write(sd, mesaj.c_str(), lgMesaj);

            pair<string, string> per((string)user2, (string)username);
            ref.push_back(per);
        }
        step = sqlite3_step(stmt);
    }

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";

    for (auto it : acc)
    {
        cout << "Am sters din acc\n";
        sql = "DELETE FROM FRND_REQUESTS WHERE USERNAME1 LIKE '" + it.first + "' AND USERNAME2 LIKE '"
                + it.second + "';";
        printf("Apel SQL: %s\n", sql.c_str());
        char* err;
        //rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, &err);
        rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &err);
        if (rc != SQLITE_OK)
            cout << err << "\n";
//        step = sqlite3_step(stmt);
//        final = sqlite3_finalize(stmt);e
//        if (final != SQLITE_OK)
//            cout << "final_gresit\n";
    }

    for (auto it : ref)
    {
        cout << "Am sters din ref\n";
        sql = "DELETE FROM FRND_REQUESTS WHERE USERNAME1 LIKE '" + it.first + "' AND USERNAME2 LIKE '"
              + it.second + "';";
        printf("Apel SQL: %s\n", sql.c_str());
        //rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, nullptr);
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        step = sqlite3_step(stmt);
        final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";
    }

    for (auto it : acc)
    {
        cout << "Am inserat din acc\n";
        sql = "INSERT INTO FRIENDS (USERNAME1, USERNAME2) VALUES('" + it.first + "', '"
              + it.second + "');";
        printf("Apel SQL: %s\n", sql.c_str());
        //rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, nullptr);
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        step = sqlite3_step(stmt);
        final = sqlite3_finalize(stmt);
        if (final != SQLITE_OK)
            cout << "final_gresit\n";
    }

//    final = sqlite3_finalize(stmt);
//    if (final != SQLITE_OK)
//        cout << "final_gresit\n";
    sqlite3_close(db);
}

void chat(int sd, int logged, char* username, char* comanda)
{
    if (logged == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

    char* apel = strtok(comanda, " \n");
    char* dest = strtok(NULL, " \n");

    if (checkUsername(dest) == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "User does not exist!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    rc = sqlite3_open("users.db", &db);

    string sql = "SELECT COUNT(*) FROM MESSAGES WHERE USERNAME1 LIKE '" + (string)username
                       + "' AND USERNAME2 LIKE '" + (string)dest + "' OR USERNAME2 LIKE '" + (string)username
                       + "' AND USERNAME1 LIKE '" + (string)dest + "';";


    //char* err;
    //rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &err);
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    int step = sqlite3_step(stmt);
//    if (step != SQLITE_OK)
//        cout << "step gresit" << "\n";
    int nrMsg = sqlite3_column_int(stmt, 0);

    if (nrMsg == 0)
    {
        int mesaje = 1;
        write(sd, &mesaje, sizeof(int));
        const char* msg = "There are no messages to be shown.\n";
        int lg = strlen(msg);
        write(sd, &lg, sizeof(int));
        write(sd, msg, lg);
        return;
    }

    printf("Numarul de mesaje este: %d\n", nrMsg);
    write(sd, &nrMsg, sizeof(int));

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";

    sql = "SELECT USERNAME1, TEXT FROM MESSAGES WHERE USERNAME1 LIKE '" + (string)username
    + "' AND USERNAME2 LIKE '" + (string)dest + "' OR USERNAME2 LIKE '" + (string)username
    + "' AND USERNAME1 LIKE '" + (string)dest + "';";

    cout << sql << '\n';

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    step = sqlite3_step(stmt);
    while (step == SQLITE_ROW)
    {
        char* src = (char*)sqlite3_column_text(stmt, 0);
        char* txt = (char*)sqlite3_column_text(stmt, 1);
        string mesaj = (string)src + " said: " + (string)txt;
        int lgMesaj = mesaj.length();
        write(sd, &lgMesaj, sizeof(int));
        write(sd, mesaj.c_str(), lgMesaj);

        step = sqlite3_step(stmt);
    }
    final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
}

void msg(int sd, int logged, char* username, char* comanda)
{
    if (logged == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You are not logged in!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

    int lgComanda = strlen(comanda);
    comanda[lgComanda - 1] = '\0';
    char* apel = strtok(comanda, " \n");
    char* dest = strtok(NULL, " \n");
    int lgApel = strlen(apel);
    int lgDest = strlen(dest);

    if (strcmp(username, dest) == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "You can't send a message to yourself!\n";
        int lg = strlen(mesaj);
        write(sd, &lg, sizeof(int));
        write(sd, mesaj, lg);
        return;
    }

//    char* text = (char*)malloc(lgComanda - lgApel - lgDest - 2);
//    strcpy(text, comanda + lgApel + lgDest + 2);
//    text[strlen(text) - 1] = '\0';

    string text = (string)(comanda + lgApel + lgDest + 2);
    text[text.length()] = '\0';

    if (checkUsername(dest) == 0)
    {
        int nrMsg = 1;
        write(sd, &nrMsg, sizeof(int));
        const char* mesaj = "User does not exist!\n";
        int lgMsg = strlen(mesaj);
        write(sd, &lgMsg, sizeof(int));
        write(sd, mesaj, lgMsg);
        return;
    }

    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    rc = sqlite3_open("users.db", &db);
    if (rc)
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "INSERT INTO MESSAGES (USERNAME1,USERNAME2,TEXT) VALUES ('" + (string)username + "', '" +
                 (string)dest + "','" + text + "');";

    cout << sql << "\n";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);
    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
    {cout << zErrMsg << '\n';
        cout << "final_gresit\n";}
    sqlite3_close(db);
    int nrMsg = 1;
    write(sd, &nrMsg, sizeof(int));
    const char* mesaj = "Message sent!\n";
    int lgMsg = strlen(mesaj);
    write(sd, &lgMsg, sizeof(int));
    write(sd, mesaj, lgMsg);
}

int nrRowsInTable(char* name)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM " + (string)name + ";";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int nr = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nr;
}

int checkAdmin(char* username)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM USERS WHERE ADMIN = 1 AND USERNAME LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int nr = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nr;
}

int countFriends(char* username)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM FRIENDS WHERE USERNAME1 LIKE '" + (string)username +
            "' OR USERNAME2 LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int nr = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nr;
}

int checkFriends(char* username, char* comanda)
{
    int lgComanda = strlen(comanda);
    char* c2 = (char*)malloc(lgComanda);
    strcpy(c2, comanda);
    char* apel = strtok(c2, " \n");
    char* dest = strtok(NULL, " \n");

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM FRIENDS WHERE USERNAME1 LIKE '" + (string)username +
                 "' AND USERNAME2 LIKE '" + (string)dest + "' OR USERNAME1 LIKE '"
                 + (string)dest + "' AND USERNAME2 LIKE '" + (string)username + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int nr = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nr;
}

int checkReq(char* username, char* comanda)
{
    int lgComanda = strlen(comanda);
    char* c2 = (char*)malloc(lgComanda);
    strcpy(c2, comanda);
    char* apel = strtok(c2, " \n");
    char* dest = strtok(NULL, " \n");

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt* stmt = NULL;

    /* Open database */
    rc = sqlite3_open("users.db", &db);

    if( rc )
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    else
        fprintf(stderr, "Opened database successfully\n");

    string sql = "SELECT COUNT(*) FROM FRND_REQUESTS WHERE USERNAME1 LIKE '" + (string)username +
                 "' AND USERNAME2 LIKE '" + (string)dest + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int step = sqlite3_step(stmt);

    if( step != SQLITE_ROW)
    {
        cout << "step gresit";
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }

    int nr = sqlite3_column_int (stmt, 0);

    int final = sqlite3_finalize(stmt);
    if (final != SQLITE_OK)
        cout << "final_gresit\n";
    sqlite3_close(db);
    return nr;
}

void showCommands(int sd)
{
    int nrMsg = 11;
    write(sd, &nrMsg, sizeof(int));

    const char* mesaj;
    mesaj = "\nUSER COMMANDS:";
    writeToClient(sd, mesaj);
    mesaj = "register <username> <password> <admin/user>";
    writeToClient(sd, mesaj);
    mesaj = "login <username> <password>";
    writeToClient(sd, mesaj);
    mesaj = "logout";
    writeToClient(sd, mesaj);
    mesaj = "post <public/friends> <text>";
    writeToClient(sd, mesaj);
    mesaj = "viewposts";
    writeToClient(sd, mesaj);
    mesaj = "add_friend <username>";
    writeToClient(sd, mesaj);
    mesaj = "char <username>";
    writeToClient(sd, mesaj);
    mesaj = "msg <username>";
    writeToClient(sd, mesaj);
    mesaj = "ADMIN COMMANDS:";
    writeToClient(sd, mesaj);
    mesaj = "check <tablename> (USERS/POSTS/FRIENDS/FRND_REQUESTS/MESSAGES)";
    writeToClient(sd, mesaj);
}

void writeToClient(int sd, const char* mesaj)
{
    int lg = strlen(mesaj);
    write(sd, &lg, sizeof(int));
    write(sd, mesaj, lg);
}