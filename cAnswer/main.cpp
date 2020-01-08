#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

using namespace std;

typedef pair<int, int> CrossPair;

template<class _T> void Txt2Map(string txtFile, map<int, _T> & mp)
{
    ifstream fin;
    fin.open(txtFile, ios_base::in);
    if(! fin.is_open()) cout << "open the road file errors!" << endl;

    int info[10];
    char buffer[200];

    fin.getline(buffer, 200); // 过滤掉第一行
    while(! fin.eof())
    {
        fin.getline(buffer, 200);
        int i1 = 1, i2 = 0, i = 0;
        while(buffer[i2] != '\0')
        {
            if(buffer[i2] == ',' || buffer[i2] == ')')
            {
                buffer[i2] = '\0';
                sscanf(buffer+i1, "%u", info+i);
                i++;
                i2++;
                i1 = i2+1;
            }
            else
            {
                i2++;
            }
        }
        _T a(info);
        mp.insert(pair<int, _T>(info[0],a)); // 会按key从小到大排序
    }
    fin.close();
}

class Car
{
public:
    int id;
    int from;
    int to;
    int speed;
    int planTime;

    static int count; // 总的对象数目
    static set<int> speedSet;

    vector<int> answer;
    vector<int> answerBackup; // 用于备份更好的答案
    int realTime; // 实际出发时间
    int realTimeBackup;
    int timeCost; // 大约时间代价，初始为realTime

    int order; // 车辆当前所在answer中的顺序，初始化为1
    int chn; // 所在车道编号，初始为-1
    int pos; // 所在道路位置，该位置与carport下标对应，出路口处位置最大，初始化为-1

    int status; // 行驶状态，0 - 未出发；1 - 在路上；2 - 到达目的地
    int dpStatus; // 调度状态，0 - 终态；1 - 等待态

public:
    Car(){}
    Car(int info[])
    {
        this->id = info[0];
        this->from = info[1];
        this->to = info[2];
        this->speed = info[3];
        this->planTime = info[4];

        count++;
        speedSet.insert(speed);

        realTime = planTime;
        timeCost = realTime;
        order = -1;
        chn = -1;
        pos = -1;

        status = 0;
        dpStatus = 1;
    }
    void UpdateAnswer()
    {
        realTimeBackup = realTime;
        answerBackup.clear();
        for(auto iter = answer.begin(); iter != answer.end(); iter++){
            answerBackup.push_back(*iter);
        }
    }

};
class Carport
{
public:
    int **m = NULL;
    int id, speed, chn, length;
    int * head; // 每个车道第一辆车的位置
    int * tail; // 每个车道最后一辆车的位置
    int firWaitCarId; // -1表示没有

    int in, out, num; // 一个时间片后进来、出去和当前所拥有的车辆
    float flowRate; // 流动指数 = (in + out)/(num + out)，如果num为0，则流动指数为1

    Carport(){}
    Carport(int info[])
    {
        this->id = info[0];
        this->length = info[1];
        this->speed = info[2];
        this->chn = info[3];

        this->m = new int *[this->chn];
        for(int i = 0; i < this->chn; i++)
        {
            this->m[i] = new int[this->length];
        }
        this->head = new int[this->chn];
        this->tail = new int[this->chn];
       // memset(this->head, -1, this->chn*sizeof(int)); // memset是以字符进行赋值，所以给整型数组进行赋值是不可取的，但是-1和0这些还是可以
        Initialing();
    }
    void Initialing()
    {
        this->firWaitCarId = -1;
        this->in = 0;
        this->out = 0;
        this->num = 0;
        this->flowRate = 1.0;
        for(int i = 0; i < this->chn; i++)
        {
            for(int j = 0; j < this->length; j++){
                this->m[i][j] = -1;
            }
            this->head[i] = -1;
            this->tail[i] = this->length;
        }
    }
    void Show()
    {
        cout << "id = " << id << ", length = " << length << ", speed = " << speed << " channel = " << chn << " firWaitCarId = " << firWaitCarId << " num = " << num << endl;
        for(int i = 0; i < this->chn; i++)
        {
            for(int j = 0; j< this->length; j++)
                cout << this->m[i][j] << ' ';
            cout << endl;
        }
        cout << "head[]: ";
        for(int i = 0; i < this->chn; i++)
            cout << this->head[i] << " ";
        cout << endl;
        cout << "tail[]: ";
        for(int i = 0; i < this->chn; i++)
            cout << this->tail[i] << " ";
        cout << endl;
    }
};

class Road
{
public:
    int id;
    int length;
    int speed;
    int channel;
    int from;
    int to;
    int isDuplex;

    Carport carport1; // from 2 to
    Carport carport2; // to 2 from

    static int count;

public:
    Road(){}

    Road(int info[]): carport1(info), carport2(info)
    {
        this->id = info[0];
        this->length = info[1];
        this->speed = info[2];
        this->channel = info[3];
        this->from = info[4];
        this->to = info[5];
        this->isDuplex = info[6];

        count++;
    }

    Carport& GetCarportStartFrom(int crossId){
        if(crossId == from){ return carport1;}
        if(crossId == to && isDuplex == 1){ return carport2;}
        cout << "roadId = " << id << ", crossId = " << crossId << " GetCarportStartFrom() Error!" << endl;
        getchar();
    }

    Carport& GetCarportAheadTo(int crossId){
        if(crossId == from && isDuplex == 1){ return carport2;}
        if(crossId == to){ return carport1;}
        cout << "roadId = " << id << ", crossId = " << crossId << " GetCarportAheadTo() Error!" << endl;
        getchar();
    }
};

class Cross
{
public:
    int id;
    // 存放路口各道路的位置关系信息，key - 道路id，value - 三个元素的数组，分别存放左转/直行/右转将到达的道路id
    map<int, int *> locRelaInfo;
    int roadIdAsc[4]; // 按升序排列的道路id
    int roadNum; // 道路条数

    static int count;

public:
    Cross(){}
    Cross(int info[])
    {
        id = info[0];

        // 计算位置关系
        for(int i = 1; i < 5; i++){
            if(info[i] != -1) {
                int * t = new int[3]{info[(i)%4+1],info[(i+1)%4+1],info[(i+2)%4+1]};
                locRelaInfo.insert(make_pair(info[i], t));
            }
        }

        int temp[4] = {info[1],info[2],info[3],info[4]};
        sort(temp, temp+4); // 默认升序排列
        int i = 0;
        while(i < 4 && temp[i] == -1) i++;
        roadNum = 0;
        for(; i < 4; i++){
            roadIdAsc[roadNum] = temp[i];
            roadNum++;
        }

        count++;
    }
};

int Road::count = 0;
int Cross::count = 0;
int Car::count = 0;
set<int> Car::speedSet;

map<int, Car> carMap;
map<int, Road> roadMap;
map<int, Cross> crossMap;
vector<vector<int> > carDispatchOrder; // 车辆调度顺序 <carid, realTime>

int notDestCar;
set<int> onRoadCar;

bool comp(const vector<int> & a, const vector<int> & b)
{
    if(a[1] == b[1]){
        return a[0] < b[0];
    }
    return a[1] < b[1];
}

bool comp1(const Car * a, const Car * b)
{
    return a->timeCost < b->timeCost;
}

// 这里会需要考虑车辆之前的状态是终态还是等待态
// 仅移动能够确定终态位置的车辆
/**之前没考虑的情况：因为MoveFirWaitCar1中车辆可能在路口停下，所以此方法会导致后面的车移动不了，使不得*/
void MoveOneChannel(Carport & carport, int chn, int & waitCar, int & stepNum)
{
    int lc_speed, lc_t; // 允许的最大行进速度
    int flag[2] = {-1, carport.length}; // 下一辆车的状态以及位置, flag[0] = 0（终态） or 1（等待态）or -1（路口阻挡）
    for(int i = carport.head[chn]; i >= carport.tail[chn]; i--){
        if(carport.m[chn][i] != -1){ // 当前位置有一辆车
            Car & car = carMap[carport.m[chn][i]];
            if(car.dpStatus == 0){ // 车辆为终态
                flag[0] = 0;
                flag[1] = i;
            }
            else{ // 车辆为等待态
                lc_speed = (car.speed < carport.speed) ? car.speed : carport.speed;
                if(lc_speed > flag[1]-i-1){ //前方有阻挡
                    if(flag[0] == 0){ // 车辆阻挡，且阻挡车辆为终态，调整当前车辆位置并设置为终态
                        stepNum += flag[1]-1-i;
                        carport.m[chn][i] = -1; // 这一行应该在下一行之前，防止i=flag[1]-1时两次修改同一个位置出错
                        carport.m[chn][flag[1]-1] = car.id;
                        car.dpStatus = 0; //终态
                        car.pos = flag[1]-1;
                        //flag[0] = 0;
                        flag[1] -= 1;

                        waitCar--;
                        //EC.insert(car.id);
                    }
                    else if(flag[0] == 1){ // 车辆阻挡，且阻挡车辆为等待态，设置当前车辆为等待态
                        flag[1] = i;
                    }
                    else{ // flag[0] == -1, 路口阻挡
                        if(car.order == car.answer.size()-1) { // 即将到达目的地，调整位置和车辆状态
                            stepNum += car.answer.size()-1-i;
                            carport.m[chn][i] = -1;
                            carport.out++;
                            carport.num--;
                            car.pos = carport.length;
                            car.order = car.answer.size();
                            car.status = 2; // 车辆到达目的地
                            car.dpStatus = 0;
                            onRoadCar.erase(car.id);
                            waitCar--;
                            notDestCar--;
                            //WC.erase(car.id);
                        }
                        else{
                            flag[0] = 1;
                            flag[1] = i;
                        }
                    }
                }
                else{ // 前方没有阻挡，调整当前车辆位置并设为终态
                    // 更新车道信息
                    stepNum += lc_speed;
                    carport.m[chn][i] = -1;
                    carport.m[chn][i+lc_speed] = car.id;
                    // 更新车辆状态信息
                    car.dpStatus = 0; //终态
                    car.pos = i+lc_speed;

                    flag[0] = 0;
                    flag[1] = i+lc_speed;

                    waitCar--;
                    //EC.insert(car.id);
                }
            }
        }
    }
    // 更新该车道头尾车辆位置
    lc_t = carport.length-1;
    while(lc_t>-1 && carport.m[chn][lc_t] == -1) lc_t--;
    carport.head[chn] = lc_t;
    lc_t = 0;
    while(lc_t<carport.length && carport.m[chn][lc_t] == -1) lc_t++;
    carport.tail[chn] = lc_t;

    // 找到第一等待车辆id
    int lc_pos = -1, lc_np;
    for(int chn = 0; chn < carport.chn; chn++){
        lc_np = carport.head[chn];
        if(lc_np > lc_pos && carMap[carport.m[chn][lc_np]].dpStatus == 1){
            carport.firWaitCarId = carport.m[chn][lc_np];
            lc_pos = lc_np;
        }
    }
    if(lc_pos == -1) carport.firWaitCarId = -1;
}

// 第一次调整，对所有得道路所有车道的所有车辆，如果通过路口即到达目的地也将它设为终态
// 这里不会考虑车辆之前是终态还是等待态
void FirstAdjust(Carport & carport, int & waitCar, int & firstMoveStep)
{
    int lc_speed, lc_t;
    int flag[2];
    for(int chn = 0; chn < carport.chn; chn++)
    {
        flag[0] = -1;
        flag[1] = carport.length; // 下一辆车的状态以及位置, flag[0] = 0（终态） or 1（等待态）or -1（路口阻挡）
        for(int i = carport.head[chn]; i >= carport.tail[chn]; i--){
            if(carport.m[chn][i] != -1){ // 当前位置有一辆车
                Car & car = carMap[carport.m[chn][i]];
                lc_speed = (car.speed < carport.speed) ? car.speed : carport.speed;
                if(lc_speed > flag[1]-i-1){ //前方有阻挡
                    if(flag[0] == 0){ // 车辆阻挡，且阻挡车辆为终态，调整当前车辆位置并设置为终态
                        firstMoveStep += flag[1]-i-1;
                        carport.m[chn][i] = -1; // 这一行应该在下一行之前，防止i=flag[1]-1时两次修改同一个位置出错
                        carport.m[chn][flag[1]-1] = car.id;
                        car.dpStatus = 0; //终态
                        car.pos = flag[1]-1;
                        //flag[0] = 0;
                        flag[1] -= 1;
                    }
                    else if(flag[0] == 1){ // 车辆阻挡，且阻挡车辆为等待态，设置当前车辆为等待态
                        car.dpStatus = 1; // 等待态
                        //flag[0] = 1;
                        flag[1] = i;
                        waitCar++;
                        //WC.insert(car.id);
                    }
                    else{ // flag[0] == -1, 路口阻挡，设为等待状态
                        car.dpStatus = 1;
                        flag[0] = 1;
                        flag[1] = i;
                        waitCar++;
                        //WC.insert(car.id);
                    }
                }
                else{ // 前方没有阻挡，调整当前车辆位置并设为终态
                    // 更新车道信息
                    firstMoveStep += lc_speed;
                    carport.m[chn][i] = -1;
                    carport.m[chn][i+lc_speed] = car.id;
                    // 更新车辆状态信息
                    car.dpStatus = 0; //终态
                    car.pos = i+lc_speed;

                    flag[0] = 0;
                    flag[1] = i+lc_speed;
                }
            }
        }
        // 更新该车道头尾车辆位置
        lc_t = carport.length-1;
        while(lc_t>-1 && carport.m[chn][lc_t] == -1) lc_t--;
        carport.head[chn] = lc_t;
        lc_t = 0;
        while(lc_t<carport.length && carport.m[chn][lc_t] == -1) lc_t++;
        carport.tail[chn] = lc_t;
    }
    // 找到第一等待车辆id
    int lc_pos = -1, lc_np;
    for(int chn = 0; chn < carport.chn; chn++){
        lc_np = carport.head[chn];
        if(lc_np > lc_pos && carMap[carport.m[chn][lc_np]].dpStatus == 1){
            carport.firWaitCarId = carport.m[chn][lc_np];
            lc_pos = lc_np;
        }
    }
    if(lc_pos == -1) carport.firWaitCarId = -1;
}

// 判断道路在路口左转0/直行1还是右转2，与第一等待车辆有关；-1表示道路是从路口出发的单行道或者没有等待车辆，不会产生方向
// 即将到达终点按直行处理，也参与优先级排序
int GetRoadDirection(Road & road, Cross & cross)
{
    if(road.to != cross.id && road.isDuplex != 1) // 该道路是单行道且不朝向路口
        return -1;

    Carport & carport = road.GetCarportAheadTo(cross.id);
    // 没有第一等待车辆
    if(carport.firWaitCarId == -1)
        return -1;

    Car & car = carMap[carport.firWaitCarId];
    if(car.order == car.answer.size()-1)
        return 1;
    int rid_next = car.answer[car.order+1]; // 吓一跳道路id
    int * temp = cross.locRelaInfo[road.id];
    for(int i = 0; i < 3; i++){
        if(temp[i] == rid_next)
            return i;
    }
    cout << "Error in GetRoadDirection()" << endl;
    getchar();
}

// 移动第一等待车辆，汽车还没到终点情况，将汽车从fromCp移动到toCp，如果发生移动，则返回汽车之前所在的车道编号，如果不发生移动则返回-1
int MoveFirWaitCar1(Car & car, Carport & fromCp, Carport & toCp, int & waitCar, int & stepNum)
{
    //cout << "In MoveFirWaitCar1() looking carport:" << endl;
    //fromCp.Show();
    //toCp.Show();
    int s1 = fromCp.length-car.pos-1;
    int v2 = (car.speed<toCp.speed) ? car.speed : toCp.speed;
    int s2 = (s1<v2) ? (v2-s1) : 0;
    int lc_pos, lc_newPos, lc_oldChn, i;
    if(s2 > 0) {// 可以通过路口
        for(int chn = 0; chn < toCp.chn; chn++){
            lc_pos = toCp.tail[chn];
            if(lc_pos == toCp.length || s2-1 < lc_pos){ // 车道没车或者前方车辆位置不影响本车
                lc_newPos = ((s2-1) < lc_pos) ? (s2-1) : (lc_pos-1);
                lc_oldChn = car.chn;

                fromCp.m[lc_oldChn][car.pos] = -1;
                i = car.pos-1;
                while(i>-1 && fromCp.m[lc_oldChn][i] == -1) i--;
                fromCp.head[lc_oldChn] = i;
                if(i == -1)
                    fromCp.tail[lc_oldChn] = fromCp.length;
                fromCp.out++;
                fromCp.num--;
                /** 本来这时应该更新fromCp.firWaitCarId，但是MoveOneChannel()之后又要更新一次，
                此函数移动成功之后必然会调用MoveOneChannel()函数，所以就只在那里更新就行。
                MoveFirWaitCar2()也一样*/

                toCp.m[chn][lc_newPos] = car.id;
                toCp.tail[chn] = lc_newPos;
                if(toCp.head[chn] == -1)
                    toCp.head[chn] = lc_newPos;
                toCp.in++;
                toCp.num++;

                stepNum += s1 + lc_newPos + 1;
                car.pos = lc_newPos;
                car.dpStatus = 0;
                car.chn = chn;
                car.order++;

                waitCar--;
                //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
                //EC.insert(car.id);
                return lc_oldChn;
            }
            else if(lc_pos > 0) { // 前方有空位且有车辆阻挡
                Car & car_ahead = carMap[toCp.m[chn][lc_pos]]; // 前方车辆
                if(car_ahead.dpStatus == 0){
                    //int newPos = s2-1 < pos ? s2-1 : pos-1;
                    lc_newPos = lc_pos-1;
                    lc_oldChn = car.chn;

                    fromCp.m[lc_oldChn][car.pos] = -1;
                    i = car.pos-1;
                    while(i>-1 && fromCp.m[lc_oldChn][i] == -1) i--;
                    fromCp.head[lc_oldChn] = i;
                    if(i == -1)
                        fromCp.tail[lc_oldChn] = fromCp.length;
                    fromCp.out++;
                    fromCp.num--;
                    /** 本来这时应该更新fromCp.firWaitCarId，但是MoveOneChannel()之后又要更新一次，
                    此函数移动成功之后必然会调用MoveOneChannel()函数，所以就只在那里更新就行。
                    MoveFirWaitCar2()也一样*/

                    toCp.m[chn][lc_newPos] = car.id;
                    toCp.tail[chn] = lc_newPos;
                    //if(toCp.head[chn] == -1) // 这里感觉可以省，前方一定有车辆
                    //    toCp.head[chn] = newPos;
                    toCp.in++;
                    toCp.num++;

                    stepNum += s1 + lc_newPos + 1;
                    car.pos = lc_newPos;
                    car.dpStatus = 0;
                    car.chn = chn;
                    car.order++;

                    waitCar--;
                    //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
                    //EC.insert(car.id);
                    return lc_oldChn;
                }
                return -1; // 前方空位车道车辆为等待态，不移动
            }
            else{ // lc_pos == 0, 前方没有空位
                Car & car_ahead = carMap[toCp.m[chn][lc_pos]]; // 前方车辆
                if(car_ahead.dpStatus == 1){ // 前方车道尾部车辆为等待态，不移动
                    return -1;
                }
                // 否则，查看下一车道
            }
        }
    }// end if
    // 不能通过路口：s2 == 0，或者
    // 前方道路所有车道没有空位且阻挡车辆为终态（因为如果阻挡车辆为等待态那么上一个循环体就会直接返回了），
    // 移动车辆，更新车辆和道路状态
    lc_newPos = fromCp.length-1;
    fromCp.m[car.chn][car.pos] = -1;
    fromCp.m[car.chn][lc_newPos] = car.id;
    fromCp.head[car.chn] = lc_newPos;
    if(fromCp.tail[car.chn] == car.pos)
        fromCp.tail[car.chn] = lc_newPos;

    stepNum += lc_newPos-car.pos;
    car.pos = lc_newPos;
    car.dpStatus = 0;
    waitCar--;
    //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
    //EC.insert(car.id);
    return car.chn;
}

// 移动第一等待车辆，汽车即将到达终点情况，返回汽车之前所在的车道编号，这种情况一定会发生移动
int MoveFirWaitCar2(Car & car, Carport & fromCp, int & waitCar, int & stepNum)
{
    //cout << "In MoveFirWaitCar2() looking carport:" << endl;
    //fromCp.Show();
    int speed = (car.speed<fromCp.speed) ? car.speed : fromCp.speed;
    int i;
    if(car.order == car.answer.size()-1 && speed > fromCp.length-car.pos-1){
        fromCp.m[car.chn][car.pos] = -1;
        i = car.pos-1;
        while(i>-1 && fromCp.m[car.chn][i] == -1) i--;
        fromCp.head[car.chn] = i;
        if(i == -1)
            fromCp.tail[car.chn] = fromCp.length;
        fromCp.out++;
        fromCp.num--;

        stepNum += fromCp.length-car.pos;
        car.dpStatus = 0;
        car.pos = car.answer.size();
        car.status = 2;
        car.order = car.answer.size();

        waitCar--;
        onRoadCar.erase(car.id);
        notDestCar--;
        //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
        //EC.insert(car.id);
        return car.chn;
    }
    else{
        cout << "Why the car can not arive destination in MoveChnFirWaitCar2() ?" << endl;
        getchar();
    }
}

// 移动某条路上第一优先级等待车辆通过路口，如果未移动返回-1，如果发生移动则返回该车辆之前所在车道
int MoveFirstWaitCar(Road & road, Cross & cross, int & waitCar, int & stepNum)
{
    //Carport c;
    //Carport & toCp = c;
    // 然后后文toCp = roadMap[rid_ahead].GetCarportStartFrom(cross.id)，在函数中对toCp进行修改后发现num并没有实际修改到的，导致出错，
    // 原因应该是toCp先对c做了引用，因此它有自己独立的变量空间（即chn,length,num,head,tail等），而后改为对实际
    // 道路carport引用时，只是将这些变量进行了简单地赋值，因此head,tail,m等指针变量指向了实际的道路空间，而其它整型变
    // 量还是c的空间。
    // 感觉这个和整数引用可以改为引用其它变量不一样额！
    if(road.to != cross.id && road.isDuplex != 1) // 该道路是单行道且不朝向路口
        return -1;

    Carport & fromCp = road.GetCarportAheadTo(cross.id);
    int firWaitCarId = fromCp.firWaitCarId;

    // 如果没有等待车辆，不发生任何移动，直接返回
    if(firWaitCarId == -1)
        return -1;

    int rid_cur = road.id;
    int rid_left = cross.locRelaInfo[rid_cur][0];
    int rid_ahead = cross.locRelaInfo[rid_cur][1];
    int rid_right = cross.locRelaInfo[rid_cur][2];

    Car & firWaitCar = carMap[firWaitCarId];
    //cout << "In MoveFirstWaitCar(): firWaitCar.id = " << firWaitCar.id << ", road.id = " << road.id << ", cross.id = " << cross.id << endl;

    int dire = GetRoadDirection(road, cross);
    switch(dire){
    case 0: // 左转
        if(rid_right != -1){
            int rightRoadDire = GetRoadDirection(roadMap[rid_right], cross);
            if(rightRoadDire == 1) // 冲突，应该等直行车辆先行，不能移动
                return -1;
        }
        return MoveFirWaitCar1(firWaitCar, fromCp, roadMap[rid_left].GetCarportStartFrom(cross.id), waitCar, stepNum);
    case 1: // 直行
        if(firWaitCar.order == firWaitCar.answer.size()-1)
            return MoveFirWaitCar2(firWaitCar, fromCp, waitCar, stepNum);
        else
            return MoveFirWaitCar1(firWaitCar, fromCp,roadMap[rid_ahead].GetCarportStartFrom(cross.id) , waitCar, stepNum);
        break;
    case 2: // 右转
        if(rid_left != -1){
            int leftRoadDire = GetRoadDirection(roadMap[rid_left], cross);
            if(leftRoadDire == 1) // 冲突，应该等直行车辆先行，不能移动
                return -1;
        }
        if(rid_ahead != -1){
            int aheadRoadDire = GetRoadDirection(roadMap[rid_ahead], cross);
            if(aheadRoadDire == 0) // 冲突，应该等直行车辆先行，不能移动
                return -1;
        }
        return MoveFirWaitCar1(firWaitCar, fromCp, roadMap[rid_right].GetCarportStartFrom(cross.id), waitCar, stepNum);
        break;
    }
    cout << "Error in MoveFirstPriorityCar()" << endl;
    getchar();
    return -1;
}

// 这里不需要再判断前方车辆是何状态了，因为调用此函数前所有路上车辆都已经是终态
void DriveCarInGarage(int time, int & dsTh, int & thirdMoveStep)
{
    bool flag = false; // 是否已经遇到了堵死的车辆
    for(int i = dsTh; i < carDispatchOrder.size(); i++){
        if(carDispatchOrder[i][1] > time){
            if(flag == false) dsTh = i;
            return;
        }
        if(carDispatchOrder[i][2] == 1)
            continue;
        Car & car = carMap[carDispatchOrder[i][0]];
        Road & road = roadMap[car.answer[0]];
        //cout << "In DriveCarInGarage(): " << car.id << ' ' << road.id << ' ' << car.from << endl;

        Carport & carport = road.GetCarportStartFrom(car.from);
        int lc_s = (car.speed<carport.speed) ? car.speed : carport.speed;
        int chn;
        for(chn = 0; chn < carport.chn; chn++){
            if(carport.tail[chn] > 0){
                lc_s = ((lc_s-1) < carport.tail[chn]) ? (lc_s-1) : (carport.tail[chn]-1);
                thirdMoveStep += lc_s+1;
                carport.m[chn][lc_s] = car.id;
                carport.tail[chn] = lc_s;
                if(carport.head[chn] == -1) // 该车道上之前没有车辆
                    carport.head[chn] = lc_s;
                carport.in++;
                carport.num++;
                car.chn = chn;
                car.order = 0;
                car.pos = lc_s;
                car.status = 1;
                //car.dpStatus = 0; // 这里设不设终态无所谓，反正接下来马上要FirstAdjust()重新调整
                carDispatchOrder[i][2] = 1; // 该车成功出发
                onRoadCar.insert(car.id);
                break;
            }
        }
        if(flag == false && chn == carport.chn){ // 该车未出发，被堵死
            dsTh = i;
            flag = true;
        }
    }
}

// 1. 路口按道路编号从小到大让它跑起来（循环调度）；
// 2. 首先看该道路上所有能通过路口的车辆，能不能走只看是否与其它道路能通过路口的第一优先级车辆是否冲突
// 输入为car.txt,cross.txt,road.txt answer.txt， 输出为调度时间、总调度时间
// 如果死锁返回-1，成功返回1
int Judge(int& time, int& allCarTime)
{//road in id asc order; car in planTime-id asc order; cross in id asc
    //for(auto iter=cdo.begin(); iter != cdo.end(); iter++){
    //    cout << (*iter)[0] << ' ' << (*iter)[1] << ' ' << carMap[(*iter)[0]].realTime << ' ' << (*iter)[2] << endl;
    //}

    fstream fo;
    fo.open("../log_for_judge.txt", ios_base::out);
    if(! fo.is_open()) cout << "open the log_for_judge file errors!" << endl;

    notDestCar = carDispatchOrder.size();

    time = 0;
    allCarTime = 0;

    int dsTh = 0;
    int after2;
    vector<int> carForDisp;
    vector<int> carDelay;
    int firstMoveStep, secondMoveStep, thirdMoveStep;
    while(notDestCar > 0) // 还有车辆未到达目的地
    {
        time++;
        fo << time << " " << onRoadCar.size() << " " << notDestCar << endl;
        cout << time << " " << onRoadCar.size() << " " << notDestCar << endl;

//        if(time == 21){
//            fstream fo1;
//            fo1.open("../onRoadCar_for_judge.txt",ios_base::out);
//            for(auto iter = onRoadCar.begin(); iter != onRoadCar.end(); iter++)
//                fo1 << *iter << endl;
//            fo1.close();
//            return -1;
//        }

        allCarTime += onRoadCar.size(); // 当前所有在路上的车辆都要经历该时间片

        //WC.clear();
        //第一步：驱动路上所有能到达终态的车辆到达终态，并标记车辆状态，所有车辆要么标记为等待态，要么终态
        int waitCar = 0; // 等待的车辆数
        firstMoveStep = 0;
        for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++)
        {// 这个道路调度的先后没有关系
            Road & road = iter->second;
            FirstAdjust(road.carport1, waitCar, firstMoveStep);
            if(road.isDuplex == 1)
                FirstAdjust(road.carport2, waitCar, firstMoveStep);

        }
        //EC.clear();
        //第二步：处理等待的车辆
        int old = waitCar;
        while(waitCar > 0){ // 虽然等待车辆全部设为终态，但是可能因为堵塞没有产生任何移动，怎么解决？
            secondMoveStep = 0;
            for(auto iter = crossMap.begin(); iter != crossMap.end(); iter++)
            { // 按路口编号从大到小进行调度
                Cross & cross = iter->second;
                //cout << "Current dispatch cross id is : " << cross.id << endl;
                int notMove = 0; // 接连没有移动的道路数目
                while(notMove < cross.roadNum){
                    for(int i = 0; i < cross.roadNum; i++){ // 对每一条路
                        Road & road = roadMap[cross.roadIdAsc[i]];
                        int chn = MoveFirstWaitCar(road, cross, waitCar, secondMoveStep);
                        while(chn != -1) { // 第一辆车发生移动
                            notMove = 0;
                            MoveOneChannel(road.GetCarportAheadTo(cross.id), chn, waitCar, secondMoveStep);
                            chn = MoveFirstWaitCar(road, cross, waitCar, secondMoveStep);
                        }
                        notMove++;
                    }
                }
            }
            if(old == waitCar){
                fo << "dead lock" << endl;
                return -1;
            }
            old = waitCar;
        }

        thirdMoveStep = 0;
        after2 = onRoadCar.size(); // 经过该时间片调度后还在路上的车辆
        //第三步，驱动车库车辆上路
        if(dsTh < carDispatchOrder.size())
            DriveCarInGarage(time, dsTh, thirdMoveStep);

        allCarTime += onRoadCar.size() - after2; // 该时间片新上路的车辆数

        if(firstMoveStep+secondMoveStep+thirdMoveStep == 0){
            fo << "yong du" << endl;
            return -1;
        }
    }
    return 1;
}
/**17699 5017 12
17699 5017 12
17894 5017 12
17894 5017 12*/
void Restore() /**carDispatchOrder, carMap, roadMap*/
{
    /**for(auto iter = carDispatchOrder.begin(); iter != carDispatchOrder.end(); iter++){
        (*iter)[2] = -1;
    }*/
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        Car & car = iter->second;
        car.status = 0;
        car.dpStatus = 1;
        car.pos = -1;
        car.order = -1;
        car.chn = -1;
        car.answer.clear();
    }
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        road.carport1.Initialing();
        road.carport2.Initialing();
    }
}

vector<int> GetRoadStartFrom(Cross & cross, int rid)
{
    vector<int> roads;
    for(int i = 0; i < cross.roadNum; i++){
        Road & road = roadMap[cross.roadIdAsc[i]];
        if(road.isDuplex == 1 || road.from == cross.id){
            if(road.id != rid) roads.push_back(road.id);
        }
    }
    return roads;
}

// 大致检查是否所有车辆按路线是从from到to
bool Check()
{
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        Car & car = iter->second;
        if(roadMap[car.answer[0]].from != car.from && roadMap[car.answer[0]].to != car.from)
            return false;
        if(roadMap[car.answer[car.answer.size()-1]].from != car.to && roadMap[car.answer[car.answer.size()-1]].to != car.to)
            return false;
    }
    return true;
}

void LoadAnswer(string txtFile)
{
    ifstream fin;
    fin.open(txtFile, ios_base::in);
    if(! fin.is_open()) cout << "open the road file errors!" << endl;

    int info[5000];
    char buffer[50000];

    fin.getline(buffer, 50000); // 过滤掉第一行
    int line = 0;
    while(! fin.eof())
    {
        line++;
        fin.getline(buffer, 50000);
        int i1 = 1, i2 = 0, i = 0;
        while(buffer[i2] != '\0')
        {
            if(buffer[i2] == ',' || buffer[i2] == ')')
            {
                buffer[i2] = '\0';
                sscanf(buffer+i1, "%u", info+i);
                i++;
                i2++;
                i1 = i2+1;
            }
            else
            {
                i2++;
            }
        }
        carMap[info[0]].realTime = info[1];
        for(int j = 2; j < i; j++){
            carMap[info[0]].answer.push_back(info[j]);
        }
    }
    fin.close();
}

int main(int argc, char *argv[])
{
    cout << "Begin" << endl;
    string carPath("../1-map-training-1/car.txt");
    string roadPath("../1-map-training-1/road.txt");
    string crossPath("../1-map-training-1/cross.txt");
    string answerPath("../1-map-training-1/answer_with_modifyable.txt");

    cout << "carPath is " << carPath << endl;
    cout << "roadPath is " << roadPath << endl;
    cout << "crossPath is " << crossPath << endl;
    cout << "answerPath is " << answerPath << endl;

    Txt2Map(carPath, carMap);
    Txt2Map(roadPath, roadMap);
    Txt2Map(crossPath, crossMap);
    LoadAnswer(answerPath);

    /**新方法：速度大的先走，定期更新最短路矩阵**/
    int time = 0, allCarTime = 0;

    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        vector<int> temp;
        temp.push_back(iter->second.id);
        temp.push_back(iter->second.realTime);
        temp.push_back(-1);
        carDispatchOrder.push_back(temp);
    }
    sort(carDispatchOrder.begin(),carDispatchOrder.end(),comp);

    Judge(time, allCarTime);

    cout << "time = " << time << ", allCarTime = " << allCarTime << endl;

    return 0;
}
// 路口编号从1到n连续编号
// 车辆目的地和起点不重合
