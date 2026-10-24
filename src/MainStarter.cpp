#include "MainStarter.h"
#include "cmath"
#include "QDir"
#include <openssl/opensslv.h>

int gTimeOut = 3;
int gMode;
std::atomic<int> PieCamerasC1(0), PieBA(0), PieOther(0), PieSSH(0);
std::atomic<int> camerasC1(0), filtered(0), Overl(0), Alive(0), saved(0), other(0), ssh(0);
std::atomic<int> found(0), indexIP(0);
int MaxPass = 0, MaxLogin = 0,
MaxWFLogin = 0, MaxWFPass = 0,
MaxFTPLogin = 0, MaxFTPPass = 0,
MaxSSHPass = 0;
std::atomic<int> baCount(0);
std::atomic<int> cIndex(0);
int gMaxBrutingThreads = 50;
unsigned int Activity = 0;
char gTLD[128] = { 0 };
char gPorts[65536] = { 0 };
char currentIP[MAX_ADDR_LEN] = { 0 };
char finalIP[32] = { 0 };
std::atomic<int> cons(0), BrutingThrds(0), gThreads(0);
std::vector<int> MainStarter::portVector(0);
int MainStarter::flCounter = 0;
int MainStarter::gflIndex = 0;
unsigned int **MainStarter::ipsstartfl = NULL;
unsigned int **MainStarter::ipsendfl = NULL;
volatile bool MainStarter::savingBackUpFile = false;
QJsonArray *jsonArr = new QJsonArray();
bool horLineFlag = false;
bool gShuffle = true;
std::atomic<int> ipCounter(0);
long long unsigned gTargets = 0, gTargetsNumber = 1;
char currentMask[128]	= { 0 };
char metaRange[256]		= { 0 };
char metaPercent[256]	= { 0 };
char metaIPS[256]		= { 0 };
char metaTargets[256]	= { 0 };
char metaETA[256]		= { 0 };
char metaOffline[256]	= { 0 };

bool saveBackup;

void MainStarter::unBlockButtons(){
    stt->doEmitionBlockButton(false);
}
int MainStarter::fileLoader(const char *fileName) {
    FILE *fl = fopen(fileName, "r");
    if (fl == NULL) {
        stt->doEmitionRedFoundData("[IP Loader] Cannot open IP list.");
        return -1;
    }

    unsigned int importFileSize = 0;
    char line[256];
    while (fgets(line, sizeof(line), fl) != NULL) {
        char *p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') continue;
        if (p[0] == '/' && p[1] == '/') continue;
        ++importFileSize;
    }
    if (importFileSize == 0) {
        fclose(fl);
        return -1;
    }

    rewind(fl);
    ipsstartfl = new unsigned int*[importFileSize];
    ipsendfl = new unsigned int*[importFileSize];
    for (unsigned int i = 0; i < importFileSize; ++i) {
        ipsstartfl[i] = new unsigned int[4];
        ipsendfl[i] = new unsigned int[4];
    }

    gTargets = 0;
    while (fgets(line, sizeof(line), fl) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        char *p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0' || *p == '#' || (p[0] == '/' && p[1] == '/')) continue;
        if (strstr(p, "RESTORE_IMPORT_SESSION") != NULL) continue;

        unsigned int a[4] = {0}, b[4] = {0};
        if (strchr(p, '-') != NULL) {
            if (sscanf(p, "%u.%u.%u.%u-%u.%u.%u.%u",
                       &a[0], &a[1], &a[2], &a[3], &b[0], &b[1], &b[2], &b[3]) != 8) {
                stt->doEmitionRedFoundData("[IP Loader]Wrong list format. String-> [" +
                                           QString::fromLocal8Bit(p) + "]");
                continue;
            }
        } else if (strchr(p, '/') != NULL) {
            unsigned int mask = 0;
            if (sscanf(p, "%u.%u.%u.%u/%u", &a[0], &a[1], &a[2], &a[3], &mask) != 5 || mask > 32) {
                stt->doEmitionRedFoundData("[IP Loader]Wrong list format. String-> [" +
                                           QString::fromLocal8Bit(p) + "]");
                continue;
            }
            const unsigned long long ip = (static_cast<unsigned long long>(a[0]) << 24)
                    | (static_cast<unsigned long long>(a[1]) << 16)
                    | (static_cast<unsigned long long>(a[2]) << 8)
                    | a[3];
            const unsigned long long netmask = (mask == 0)
                    ? 0ULL
                    : (0xFFFFFFFFULL << (32 - mask)) & 0xFFFFFFFFULL;
            const unsigned long long minIp = ip & netmask;
            const unsigned long long maxIp = minIp | (~netmask & 0xFFFFFFFFULL);
            a[0] = static_cast<unsigned int>(minIp >> 24);
            a[1] = static_cast<unsigned int>((minIp >> 16) & 0xFF);
            a[2] = static_cast<unsigned int>((minIp >> 8) & 0xFF);
            a[3] = static_cast<unsigned int>(minIp & 0xFF);
            b[0] = static_cast<unsigned int>(maxIp >> 24);
            b[1] = static_cast<unsigned int>((maxIp >> 16) & 0xFF);
            b[2] = static_cast<unsigned int>((maxIp >> 8) & 0xFF);
            b[3] = static_cast<unsigned int>(maxIp & 0xFF);
        } else {
            stt->doEmitionRedFoundData("[IP Loader]Wrong list format. String-> [" +
                                       QString::fromLocal8Bit(p) + "]");
            continue;
        }

        const unsigned long long ip1 = (static_cast<unsigned long long>(a[0]) << 24)
                | (static_cast<unsigned long long>(a[1]) << 16)
                | (static_cast<unsigned long long>(a[2]) << 8)
                | a[3];
        const unsigned long long ip2 = (static_cast<unsigned long long>(b[0]) << 24)
                | (static_cast<unsigned long long>(b[1]) << 16)
                | (static_cast<unsigned long long>(b[2]) << 8)
                | b[3];
        if (ip1 > ip2) {
            stt->doEmitionRedFoundData("Malformed input: check your range (" +
                                       QString::fromLocal8Bit(p) + ")");
            continue;
        }

        const unsigned int row = MainStarter::flCounter;
        if (row >= importFileSize) break;
        for (int k = 0; k < 4; ++k) {
            ipsstartfl[row][k] = a[k];
            ipsendfl[row][k] = b[k];
        }
        gTargets += ip2 - ip1 + 1;
        ++MainStarter::flCounter;
    }
    fclose(fl);

    if (MainStarter::flCounter == 0) return -1;
    gTargetsNumber = gTargets;
    stt->doEmitionYellowFoundData("List loader - [OK] (" +
                                  QString::number(gTargetsNumber) + " hosts)");
    return 0;
}

int MainStarter::loadTargets(const char *data) {

    if (gMode == 0) {
        std::vector<std::string> rangeVec;

        if (strchr(data, '/') != NULL) {

            unsigned int ip[4] = { 0 }, ip_min[4] = { 0 }, ip_max[4] = { 0 }, tmp1, tmp2;
            unsigned int netmask = atoi(strchr(data, '/') + 1);

            std::vector<int> tmpIPVec = Utils::splitToIntVector(data, '.');

            for (int i = 0; i < tmpIPVec.size(); ++i) ip[i] = tmpIPVec[i];

            /*Set the bytes which won't be changed*/
            for (tmp1 = tmp2 = netmask / 8; tmp1>0; tmp1--){
                ip_min[tmp1 - 1] = ip[tmp1 - 1];
                ip_max[tmp1 - 1] = ip[tmp1 - 1];
            }
            /*Set the bytes which should be 0ed or 255ed.*/
            for (tmp1 = tmp2, ++tmp2; tmp2< 4; tmp2++){
                ip_min[tmp2] = 0;
                ip_max[tmp2] = 255;
            }
            /* Finally set the one which has to be shifted.*/
            if (tmp1 < 4){
                tmp2 = 8 - netmask % 8;
                ip_min[tmp1] = ip[tmp1] >> tmp2;
                ip_min[tmp1] <<= tmp2;
                ip_max[tmp1] = ip_min[tmp1] + pow(2, tmp2) - 1;
            }

            char newRangeString[128] = { 0 };
            sprintf(newRangeString, "%u.%u.%u.%u-%u.%u.%u.%u",
                    ip_min[0], ip_min[1], ip_min[2], ip_min[3], ip_max[0], ip_max[1], ip_max[2], ip_max[3]);
            rangeVec = Utils::splitToStrVector(std::string(newRangeString), '-');
        }
        else rangeVec = Utils::splitToStrVector(data, '-');

        std::vector<int> ip1TmpVec = Utils::splitToIntVector(rangeVec[0], '.');
        std::vector<int> ip2TmpVec = Utils::splitToIntVector(rangeVec[1], '.');

        ipsstart[0] = ip1TmpVec[0];
        ipsstart[1] = ip1TmpVec[1];
        ipsstart[2] = ip1TmpVec[2];
        ipsstart[3] = ip1TmpVec[3];

        ipsend[0] = ip2TmpVec[0];
        ipsend[1] = ip2TmpVec[1];
        ipsend[2] = ip2TmpVec[2];
        ipsend[3] = ip2TmpVec[3];

        ip1 = (ipsstart[0] * 16777216) +
                (ipsstart[1] * 65536) +
                (ipsstart[2] * 256) +
                ipsstart[3];
        ip2 = (ipsend[0] * 16777216) +
                (ipsend[1] * 65536) +
                (ipsend[2] * 256) +
                ipsend[3];

        if (ip1 > ip2) {
            stt->doEmitionRedFoundData("Malformed input: check your range");
            return -1;
        }

        sprintf(finalIP, "%d.%d.%d.%d",
                ipsend[0], ipsend[1], ipsend[2], ipsend[3]);

        gTargets = ip2 - ip1 + 1;
        gTargetsNumber = gTargets;
    }
    else if (gMode == 1) {
        strncpy(dnsTarget, data, 256);

        gTargets = ip2 - ip1 + 1;
        gTargetsNumber = gTargets;
    }
    else {
        if (fileLoader(data) == -1) {
            stt->doEmitionRedFoundData("IP list is empty.");
            return -1;
        }
        sprintf(finalIP, "%d.%d.%d.%d",
                ipsendfl[gflIndex][0], ipsendfl[gflIndex][1], ipsendfl[gflIndex][2], ipsendfl[gflIndex][3]);
    }

    return 0;
}
int MainStarter::loadPorts(const char *data, char delim) {
    portVector = Utils::splitToIntVector(data, delim);
    for (auto elem : portVector) {
        if (elem > 65535 | elem < 0) {
            stt->doEmitionRedFoundData("Malformed input: check your ports");
            return -1;
        }
    }
    return 0;
}

void MainStarter::saveBackupToFile()
{
    char saveStr[512] = { 0 };
    char saveBuffer[4096] = { 0 };

    if (gMode == 0 || gMode == 1) {
        char endStr[128] = { 0 };

        if (gMode == 1) {
            if (currentMask[0] == '\0') {
                sprintf(endStr, "%s", currentIP);
            }
            else strcpy(endStr, currentMask);
        } else {
            if (finalIP[0] == '\0') sprintf(endStr, "%s", currentIP);
            else sprintf(endStr, "%s-%s", currentIP, finalIP);
        }

        if (endStr[0] != '\0') {
            sprintf(saveStr, "[SESSION]:%d %s %s %d %s\n",
                    gMode, endStr, gTLD, int(gThreads), gPorts);
            strcat(saveBuffer, saveStr);
            //ZeroMemory(saveStr, sizeof(saveStr));
            saveStr[0] = 0;
        };
    } else if (gMode == -1) {
        if (MainStarter::flCounter > 0) {
            if (!saveBackup) return;
            FILE *savingFile = fopen("tempIPLst.bk", "w");
            if (NULL != savingFile) {
                char ipRange[128] = { 0 };
                if (gflIndex < MainStarter::flCounter) {
                    sprintf(ipRange, "%d.%d.%d.%d-%d.%d.%d.%d\n",
                            ipsstartfl[gflIndex][0],
                            ipsstartfl[gflIndex][1],
                            ipsstartfl[gflIndex][2],
                            ipsstartfl[gflIndex][3],
                            ipsendfl[gflIndex][0],
                            ipsendfl[gflIndex][1],
                            ipsendfl[gflIndex][2],
                            ipsendfl[gflIndex][3]);
                    fputs(ipRange, savingFile);
                    //ZeroMemory(ipRange, sizeof(ipRange));
                    ipRange[0] = 0;
                }
                for (int tCounter = gflIndex + 1; tCounter < MainStarter::flCounter; ++tCounter) {
                    sprintf(ipRange, "%d.%d.%d.%d-%d.%d.%d.%d\n",
                            ipsstartfl[tCounter][0],
                            ipsstartfl[tCounter][1],
                            ipsstartfl[tCounter][2],
                            ipsstartfl[tCounter][3],
                            ipsendfl[tCounter][0],
                            ipsendfl[tCounter][1],
                            ipsendfl[tCounter][2],
                            ipsendfl[tCounter][3]);

                    fputs(ipRange, savingFile);

                    //ZeroMemory(ipRange, sizeof(ipRange));
                    ipRange[0] = 0;
                }
                fclose(savingFile);
            }
            else stt->doEmitionRedFoundData("[_saver] Cannot open file.");
        };

        sprintf(saveStr, "[SESSION]:%d RESTORE_IMPORT_SESSION %d %s\n", gMode, int(gThreads), gPorts);
        strcat(saveBuffer, saveStr);
        //ZeroMemory(saveStr, sizeof(saveStr));
        saveStr[0] = 0;
    }
    else {
        sprintf(saveStr, "[SESSION]: 0 1.1.1.1/32 0 -p80");
        strcat(saveBuffer, saveStr);
        //ZeroMemory(saveStr, sizeof(saveStr));
        saveStr[0] = 0;
    }

    sprintf(saveStr, "[SHUFFLE]:%s\n", gShuffle ? "true" : "false");
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    sprintf(saveStr, "[THREAD_DELAY]: %d\n", Threader::gThreadDelay);
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    sprintf(saveStr, "[TIMEOUT]: %d\n", gTimeOut);
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    sprintf(saveStr, "[MAXBTHR]: %d\n", gMaxBrutingThreads);
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    sprintf(saveStr, "[SYSTEMPROXYIP]: %s\n", gProxyIP);
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    sprintf(saveStr, "[SYSTEMPROXYPORT]: %s\n", gProxyPort);
    strcat(saveBuffer, saveStr);
    //ZeroMemory(saveStr, sizeof(saveStr));
    saveStr[0] = 0;

    std::string finalSaveStr(saveBuffer);
    if (finalSaveStr.size() > 0) {
        std::ofstream file("restore");
        file << finalSaveStr;
    }
    //ZeroMemory(saveBuffer, sizeof(saveBuffer));
    saveBuffer[0] = 0;
}

bool saverRunning = false;
void MainStarter::saveBK() {
    while (savingBackUpFile) Sleep(400);
    savingBackUpFile = true;
    saveBackupToFile();
    savingBackUpFile = false;
}
void MainStarter::saver()
{
    saverRunning = true;
    Sleep(1000);
    while (saveBackup && globalScanFlag)
    {
        saveBK();
        Sleep(10000);
    };
    saverRunning = false;
}

bool ipPerSecTimer = false;
void _IPPerSecTimer() {
    ipPerSecTimer = true;
    int ticks = 0;
    int ovrlIPs = 0;
    Sleep(50);

    while (globalScanFlag)
    {
        ovrlIPs += ipCounter;
        ipCounter = 0;

        sprintf(metaIPS, "%f.5", ovrlIPs / ++ticks);

        Sleep(1000);
    }
    ipPerSecTimer = false;
}


void verboseProgress(unsigned long target) {
    stt->doEmitionUpdateArc(gTargets);
    if (gTargets > 0) --gTargets;

    char targetNPers[128] = { 0 };
    float percent = (gTargetsNumber != 0 ? (100 - target / (double)gTargetsNumber * 100) : 0);

    sprintf(targetNPers, "%lu (%.1f%%)", target, percent);

    sprintf(metaTargets, "%lu", target);
    sprintf(metaPercent, "%.1f",
            percent);
}

void _connect(std::string ip) {
    if (!globalScanFlag) return;

    ++ipCounter;

    ++cons;
    Connector con;
    for (int i = 0; i < MainStarter::portVector.size(); ++i)
    {
        if (!globalScanFlag) break;
        if (con.connectToPort((char*)ip.c_str(), MainStarter::portVector[i]) == -2) break;
    };
    --cons;
    stt->doEmitionUpdateArc(gTargets);
}

void MainStarter::startIPScan(){

    switch (gShuffle) {
    case true: {
        std::vector<std::string> ipVec;
        struct in_addr tAddr;

        unsigned long offset = ip2 - ip1;
        unsigned int limit = 0;

        if (offset == 0) {
            tAddr.s_addr = ntohl(ip1);
            ipVec.push_back(inet_ntoa(tAddr));
            ++indexIP;

            strcpy(currentIP, ipVec[0].c_str());
            ipVec.erase(ipVec.begin());
            verboseProgress(gTargets);

            Threader::fireThread(currentIP, _connect);
        }
        else {
            if (offset < 1000) {
                limit = 1;
            }
            else if (offset < 10000){
                limit = 10;
            }
            else if (offset < 100000){
                limit = 100;
            }
            else if (offset < 1000000){
                limit = 1000;
            }
            else if (offset < 10000000){
                limit = 10001;
            }
            else if (offset < 100000000){
                limit = 100011;
            }
            else if (offset < 5000000000){
                limit = 1000111;
            }

            unsigned long long int firstLoopLimit = (limit == 1 ? ip1 : ip1 + (long long)limit);

            for (unsigned long i = ip1; i <= firstLoopLimit; ++i) {
                if (!globalScanFlag) break;

                unsigned long long curIP = i;
                while (curIP <= ip2) {
                    tAddr.s_addr = ntohl(curIP);
                    ipVec.push_back(inet_ntoa(tAddr));
                    curIP += (long long)limit;
                }

                std::random_shuffle(ipVec.begin(), ipVec.end());
                while (ipVec.size() != 0) {

                    while (cons >= gThreads && globalScanFlag) Sleep(500);
                    if (!globalScanFlag) goto haters_gonna_hate_IPM;

                    ++indexIP;

                    strcpy(currentIP, ipVec[0].c_str());
                    ipVec.erase(ipVec.begin());
                    verboseProgress(gTargets);

                    Threader::fireThread(currentIP, _connect);
                }
            }
        }
haters_gonna_hate_IPM:;
        break;
    }
    case false: {
        struct in_addr tAddr;
        for (unsigned long i = ip1; i <= ip2; ++i) {

            while (cons >= gThreads && globalScanFlag) Sleep(500);
            if (!globalScanFlag) break;

            ++indexIP;

            tAddr.s_addr = ntohl(i);
            strcpy(currentIP, inet_ntoa(tAddr));
            verboseProgress(gTargets);

            Threader::fireThread(currentIP, _connect);
        }
        break;
    }
    }
}

char charAll[38] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '-',
                     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                     'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };
int _getPos(char l) {
    for (int i = 0; i < 38; ++i)
    {
        if (l == charAll[i]) return i;
    };
    return -1;
}
int _GetDNSFromMask(char *mask, char *saveMask, char *saveMaskEnder) {

    if (strchr(mask, '[') != NULL)
    {
        char maskEnd[1024] = { 0 };
        char maskRes[1024] = { 0 };
        char *ptr1 = strchr(mask, '[');
        char *ptr2 = strchr(ptr1, ']');
        int sz = ptr2 - ptr1;
        char chunk[8] = { 0 };
        strncpy(chunk, ptr1, sz + 1);
        int firstPos = _getPos(chunk[1]);
        int secondPos = _getPos(chunk[2]);

        if (firstPos > secondPos)
        {
            stt->doEmitionRedFoundData("Error at mask (Wrong letter sequence)");
            return -1;
        };

        char maskStart[1024] = { 0 };
        int startPosition = strchr(mask, '[') - mask;
        strncpy(maskStart, mask, startPosition);

        char maskEntry[1024] = { 0 };
        if (saveMaskEnder != NULL)
        {
            int startSavePosition = strchr(saveMaskEnder, '[') - saveMaskEnder;
            strncpy(maskEntry, saveMaskEnder, startSavePosition);
        };

        int szMask = strlen(mask);
        int szOffset = startPosition + 2;
        if (szMask != szOffset) strcpy(maskEnd, strchr(mask, ']') + 1);
        else maskEnd[0] = 0;

        char maskSaver[128] = { 0 };
        if (firstPos != -1 && secondPos != -1)
        {
            for (int i = firstPos; i <= secondPos; ++i)
            {
                if (!globalScanFlag) break;

                strcpy(maskSaver, saveMask);
                strcat(maskSaver, maskEntry);
                chunk[1] = charAll[i];
                //strcat(maskSaver, chunk);
                strcpy(maskRes, maskStart);
                maskRes[startPosition] = charAll[i];
                maskRes[startPosition + 1] = '\0';
                strcat(maskRes, maskEnd);

                if (_GetDNSFromMask(maskRes, maskSaver, NULL) == -1) return -1;

                maskSaver[0] = 0;
                maskRes[0] = 0;
                /*ZeroMemory(maskSaver, sizeof(maskSaver));
                ZeroMemory(maskRes, sizeof(maskRes));*/
            };
        };
    }
    else
    {
        strcpy(currentMask, saveMask);
        while (cons >= gThreads && globalScanFlag) Sleep(300);
        if (!globalScanFlag) return 0;

        ++indexIP;
        sprintf(currentIP, "%s%s", mask, gTLD);
        verboseProgress(gTargets);

        Threader::fireThread(currentIP, _connect);
    };
}
int _getChunkCount(char *data) {
    int firstPos = _getPos(data[1]);
    int secondPos = _getPos(data[2]);
    return secondPos - firstPos + 1;
}
void MainStarter::startDNSScan(){
    char dataEntry[1024] = { 0 };
    int innerCounter = 0;
    int sz = strlen(dnsTarget);

    for (int i = 0; i < sz; ++i)
    {
        if (dnsTarget[i] == '\\')
        {
            if (dnsTarget[i + 1] == 'd')
            {
                strcat(dataEntry, "[09]");
                ++i;
                innerCounter += 4;
                continue;
            }
            else if (dnsTarget[i + 1] == 'w')
            {
                strcat(dataEntry, "[0z]");
                ++i;
                innerCounter += 4;
                continue;
            }
            else if (dnsTarget[i + 1] == 'l')
            {
                strcat(dataEntry, "[az]");
                ++i;
                innerCounter += 4;
                continue;
            }
            else
            {
                QString errStr = "Error at mask (Position:" + QString::number(i + 1);
                errStr += ") \"";
                errStr += QString(dnsTarget).mid(0, i == 0 ? 0 : i);
                errStr += "<u>";
                errStr += QString(dnsTarget).mid(i, i == 0 ? i + 2 : i + 1);
                errStr += "</u>";
                errStr += QString(dnsTarget).mid(i + 2, sz);
                errStr += "\"";

                stt->doEmitionRedFoundData(errStr);
                return;
            };
        }
        else
        {
            memset(dataEntry + innerCounter++, dnsTarget[i], 1);
        };
    };

    memset(dataEntry + innerCounter + 1, '\0', 1);
    strcpy(currentMask, dataEntry);

    for (int i = 0; i < sz; ++i)
    {
        if (dataEntry[i] == '[')
        {
            for (int j = i + 1; j < i + 3; ++j)
            {
                if ((dataEntry[j] < '0' || dataEntry[j] > '9')
                        && (dataEntry[j] < 'a' || dataEntry[j] > 'z')
                        && dataEntry[j] != '_'
                        && dataEntry[j] != '-'
                        )
                {
                    QString errStr = "Error at mask (" + QString::number(j - 1);
                    errStr += ") \"";
                    errStr += QString(dataEntry).mid(0, j - 1);
                    errStr += "<u>";
                    errStr += QString(dataEntry).mid(j - 1, j + 1);
                    errStr += "</u>";
                    errStr += QString(dataEntry).mid(j + 1, strlen(dataEntry));
                    errStr += "\"";

                    stt->doEmitionRedFoundData(errStr);
                    return;
                };
            };
            i += 3;
        }
        else if (dataEntry[i] == ']')
        {
            QString errStr = "Error at mask (" + QString::number(i - 1);
            errStr += ") \"";
            errStr += QString(dataEntry).mid(0, i - 1);
            errStr += "<u>";
            errStr += QString(dataEntry).mid(i - 1, i + 1);
            errStr += "</u>";
            errStr += QString(dataEntry).mid(i + 1, strlen(dataEntry));
            errStr += "\"";

            stt->doEmitionRedFoundData(errStr);
            return;
        };
    };

    unsigned long long dnsCounter = 1;
    char *dnsPtr1 = strchr(dataEntry, '[');
    while (dnsPtr1 != NULL)
    {
        dnsCounter *= _getChunkCount(dnsPtr1);
        dnsPtr1 = strchr(dnsPtr1 + 1, '[');
    };

    gTargets = dnsCounter;
    gTargetsNumber = gTargets;
    stt->doEmitionYellowFoundData("Starting DNS-scan...");

    int y = _GetDNSFromMask(dataEntry, "", NULL);
    if (y == -1)
    {
        stt->doEmitionRedFoundData("DNS-Mode error");
    };
}

void MainStarter::startImportScan(){

    if (MainStarter::flCounter == 0)
    {
        stt->doEmitionRedFoundData("Empty IP list.");
        globalScanFlag = false;
        stt->doEmitionKillSttThread();
        return;
    };

    struct in_addr tAddr;

    switch (gShuffle) {
    case true: {
        int ipOffset = 6;
        for (gflIndex = 0; gflIndex < MainStarter::flCounter; gflIndex += ipOffset)
        {
            int ipGap = MainStarter::flCounter - gflIndex;
            if (ipGap < ipOffset)
            {
                ipOffset = ipGap;
            };

            std::vector<IPRangeHolder> ipRangeVec;
            for (int j = gflIndex; j < gflIndex + ipOffset; ++j)
            {
                /*sprintf(metaRange, "%d.%d.%d.%d-%d.%d.%d.%d",
                               ipsstartfl[j][0], ipsstartfl[j][1], ipsstartfl[j][2], ipsstartfl[j][3],
                               ipsendfl[j][0], ipsendfl[j][1], ipsendfl[j][2], ipsendfl[j][3]);*/
                ip1 = (ipsstartfl[j][0] * 16777216) +
                        (ipsstartfl[j][1] * 65536) +
                        (ipsstartfl[j][2] * 256) +
                        ipsstartfl[j][3];
                ip2 = (ipsendfl[j][0] * 16777216) +
                        (ipsendfl[j][1] * 65536) +
                        (ipsendfl[j][2] * 256) +
                        ipsendfl[j][3];

                IPRangeHolder holder;
                holder.ip1 = ip1;
                holder.ip2 = ip2;
                ipRangeVec.push_back(holder);
            }
            IPRandomizer ipRandomizer(ipRangeVec, 20000);

            int ip = 0;
            while ((ip = ipRandomizer.getNext()) != 0) {
                while (cons >= gThreads && globalScanFlag) Sleep(500);
                if (!globalScanFlag) goto haters_gonna_hate_IM;

                ++indexIP;

                tAddr.s_addr = ntohl(ip);
                strcpy(currentIP, inet_ntoa(tAddr));
                verboseProgress(gTargets);

                Threader::fireThread(currentIP, _connect);
            }
        }
haters_gonna_hate_IM:;
        break;
    }
    case false: {
        for (gflIndex = 0; gflIndex < MainStarter::flCounter; gflIndex++) {
            if (!globalScanFlag) break;
            ip1 = (ipsstartfl[gflIndex][0] * 16777216) +
                    (ipsstartfl[gflIndex][1] * 65536) +
                    (ipsstartfl[gflIndex][2] * 256) +
                    ipsstartfl[gflIndex][3];
            ip2 = (ipsendfl[gflIndex][0] * 16777216) +
                    (ipsendfl[gflIndex][1] * 65536) +
                    (ipsendfl[gflIndex][2] * 256) +
                    ipsendfl[gflIndex][3];
            struct in_addr tAddr;
            for (unsigned long i = ip1; i <= ip2; ++i) {

                while (cons >= gThreads && globalScanFlag) Sleep(500);
                if (!globalScanFlag) break;

                ++indexIP;

                tAddr.s_addr = ntohl(i);
                strcpy(currentIP, inet_ntoa(tAddr));
                verboseProgress(gTargets);
                Threader::fireThread(currentIP, _connect);
            }
        }
        break;
    };
    }
}

void MainStarter::runAuxiliaryThreads() {

    FileUpdater::loadOnce();
    if (!FileUpdater::running) {
        std::thread lpThread(FileUpdater::updateLists);
        lpThread.detach();
    }
    if (!ipPerSecTimer) {
        std::thread timerThread(_IPPerSecTimer);
        timerThread.detach();
    }
    if (!saverRunning) {
        std::thread saverThread(&MainStarter::saver, this);
        saverThread.detach();
    }
}

void MainStarter::createResultFiles() {
    char fileName[256] = { 0 };
    sprintf(fileName, "./" DIR_NAME "%s_%s", Utils::getStartDate().c_str(), Utils::getCurrentTarget().c_str());
    QDir myDir("");
    bool res = myDir.mkpath(fileName);

    if (res) {
        stt->doEmitionGreenFoundData("Result directory \"" + QString::fromLocal8Bit(fileName) + "\" successfully created.");
        return;
    }

#ifdef _WIN32
    int err = GetLastError();
    const int alreadyExists = 183; // ERROR_ALREADY_EXISTS
#else
    int err = errno;
    const int alreadyExists = EEXIST;
#endif

    if (err == alreadyExists) {
        stt->doEmitionYellowFoundData("Directory \"" + QString::fromLocal8Bit(fileName) + "\" already exists. Err: " + QString::number(err));
        return;
    }

    for (int attempt = 0; attempt < 5; ++attempt) {
        stt->doEmitionRedFoundData("Failed to create \"" + QString::fromLocal8Bit(fileName) + "\" Err: " + QString::number(err));
        Sleep(1000);
        res = myDir.mkpath(fileName);
        if (res) {
            stt->doEmitionGreenFoundData("Result directory \"" + QString::fromLocal8Bit(fileName) + "\" successfully created.");
            return;
        }
#ifdef _WIN32
        err = GetLastError();
#else
        err = errno;
#endif
    }

    stt->doEmitionRedFoundData("Failed to create \"" + QString::fromLocal8Bit(fileName) +
                               "\" after 5 attempts. Last error: " + QString::number(err));
}

/* This array will store all of the mutexes available to OpenSSL. */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static MUTEX_TYPE *mutex_buf = NULL;

static void locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
    return ((unsigned long)THREAD_ID);
}

int thread_setup(void)
{
    int i;

    mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
    if (!mutex_buf)
        return 0;
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_SETUP(mutex_buf[i]);
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    return 1;
}

int thread_cleanup(void)
{
    int i;
    if (!mutex_buf)
        return 0;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_CLEANUP(mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
    return 1;
}
#else
/* OpenSSL >= 1.1.0 manages its own locks; the legacy callbacks were removed. */
int thread_setup(void)
{
    return 1;
}

int thread_cleanup(void)
{
    return 1;
}
#endif

void MainStarter::start(const char* targets, const char* ports) {
    std::srand(std::time(NULL));

    MainStarter::flCounter = 0;
    MainStarter::gflIndex = 0;
    MainStarter::ipsstartfl = NULL;
    MainStarter::ipsendfl = NULL;
    HikVis::hikCounter = 0;
    HikVis::rviCounter = 0;
    saveBackup = true;
    curl_global_init(CURL_GLOBAL_ALL);

    thread_setup();


    QString fileSuffix = QString(targets);
    fileSuffix = fileSuffix.mid(fileSuffix.lastIndexOf("/") + 1);
    Utils::setCurrentTarget(fileSuffix.toUtf8().constData());
    createResultFiles();

    if (loadTargets(targets) == -1 ||
            loadPorts(ports, ',') == -1) {
        stt->doEmitionKillSttThread();
        return;
    }

    globalScanFlag = true;
    runAuxiliaryThreads();

    if (gMode == 0) startIPScan();
    else if (gMode == 1) startDNSScan();
    else startImportScan();

    stt->doEmitionYellowFoundData("Stopping threads...");

    // Give freshly spawned workers a chance to enter the queue before we
    // decide that nothing is running. Without this, a single-target scan can
    // finish before the worker increments `cons`.
    Sleep(1000);
    while (cons > 0 || jsonArr->size() > 0) Sleep(2000);
    MainStarter::saveBK();
    saveBackup = false;

    thread_cleanup();

    stt->doEmitionGreenFoundData("Done. Saved <u>" + QString::number(saved) +
                                 "</u> of <u>" + QString::number(found) + "</u> nodes.");
    stt->doEmitionKillSttThread();
}
