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

struct THETA
{
    int volumn = 1500;
    int space = 1;
    int randomstep = 0;
    int flow = 2;
    int T = 5;
    int backupT = 10;
};

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

    int order, orderB; // 车辆当前所在answer中的顺序，初始化为1
    int chn, chnB; // 所在车道编号，初始为-1
    int pos, posB; // 所在道路位置，该位置与carport下标对应，出路口处位置最大，初始化为-1
    int lastDire, lastDireB; // 上次转向，0 - 左转，1 - 直行，2 - 右转，-1 - 不转（没有过路口）
    int breakpoint; // 上次修改路径的位置，如果只有上次修改路径的位置小于当前位置时，才能再次修改路径

    int status, statusB; // 行驶状态，0 - 未出发；1 - 在路上；2 - 到达目的地
    int dpStatus, dpStatusB; // 调度状态，0 - 终态；1 - 等待态

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
        lastDire = -1;
        breakpoint = -1;

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
    void Restore()
    {
        status = 0;
        dpStatus = 1;
        pos = -1;
        order = -1;
        chn = -1;
        lastDire = -1;
        answer.clear();
    }

    void Backup()
    {
        orderB = order;
        chnB = chn;
        posB = pos;
        lastDireB = lastDire;
        statusB = status;
        dpStatusB = dpStatus;
    }
    void FromBackup()
    {
        order = orderB;
        chn = chnB;
        pos = posB;
        lastDire = lastDireB;
        status = statusB;
        dpStatus = dpStatusB;
    }

};
class Carport
{
public:
    int id, speed, chn, length;
    int **m, **mB;
    int * head, *headB; // 每个车道第一辆车的位置
    int * tail, *tailB; // 每个车道最后一辆车的位置
    int firWaitCarId, firWaitCarIdB; // -1表示没有

    int in, out, num; // 一个时间片后进来、出去和当前所拥有的车辆
    int inB, outB, numB;
    float flowRate, flowRateB; // 流动指数 = (in + out)/(num + out)，如果num为0，则流动指数为1

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

        this->mB = new int *[this->chn];
        for(int i = 0; i < this->chn; i++)
        {
            this->mB[i] = new int[this->length];
        }
        this->headB = new int[this->chn];
        this->tailB = new int[this->chn];
    }

    void Backup()
    {
        firWaitCarIdB = firWaitCarId;
        flowRateB = flowRate;
        inB = in;
        outB = out;
        numB = num;
        for(int i = 0; i < this->chn; i++)
        {
            for(int j = 0; j < this->length; j++){
                this->mB[i][j] = this->m[i][j];
            }
            this->headB[i] = this->head[i];
            this->tailB[i] = this->tail[i];
        }
    }
    void FromBackup()
    {
        firWaitCarId = firWaitCarIdB;
        flowRate = flowRateB;
        in = inB;
        out = outB;
        num = numB;
        for(int i = 0; i < this->chn; i++)
        {
            for(int j = 0; j < this->length; j++){
                this->m[i][j] = this->mB[i][j];
            }
            this->head[i] = this->headB[i];
            this->tail[i] = this->tailB[i];
        }
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
        for(int i = 0; i < this->chn; i++)
        {
            for(int j = 0; j< this->length; j++)
                cout << this->mB[i][j] << ' ';
            cout << endl;
        }
        cout << "headB[]: ";
        for(int i = 0; i < this->chn; i++)
            cout << this->headB[i] << " ";
        cout << endl;
        cout << "tailB[]: ";
        for(int i = 0; i < this->chn; i++)
            cout << this->tailB[i] << " ";
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

    void Restore()
    {
        carport1.Initialing();
        carport2.Initialing();
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

class Graph
{
public:
    int vNum; // 顶点数量
    set<int> speedSet; // 速度集合
    map<CrossPair, int> network;  // (crossId1, crossId2) -> roadId
    map<CrossPair, int> network1; // (roadId1, roadId2) -> crossId

    map<int, int> crossOrder; // crossId -> number

    map<int, vector<vector<float> > > lowTimeCost; // 不同车速在道路网中，任意两地之间的最少用时
    map<int, vector<vector<int> > > nextCrossForBest; // 不同车速在道路网中，从p到q最优路线时下一个要到达的路口

    Graph(map<int, Road> & roadMap, map<int, Cross> & crossMap)
    {
        this->vNum = Cross::count;
        this->speedSet = Car::speedSet;

        for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
            Road & road = iter->second;
            this->network.insert(make_pair(CrossPair(road.from,road.to),road.id));
            if(road.isDuplex == 1){
                this->network.insert(make_pair(CrossPair(road.to,road.from),road.id));
            }
        }

        int i = 1;
        for(auto iter = crossMap.begin(); iter != crossMap.end(); iter++){
            Cross & cross = iter->second;
            this->crossOrder.insert(CrossPair(cross.id, i));
            i++;
            for(int i = 0; i < cross.roadNum; i++){
                for(int j = 0; j < cross.roadNum; j++){
                    if(i != j)
                        this->network1.insert(make_pair(CrossPair(cross.roadIdAsc[i],cross.roadIdAsc[j]), cross.id));
                }
            }
        }

        for(auto iter = this->speedSet.begin(); iter != this->speedSet.end(); iter++){
            vector<vector<float> > tms;
            vector<vector<int> > nms;
            for(int i = 0; i <= this->vNum; i++){
                vector<float> tm;
                vector<int> nm;
                for(int j = 0; j <= this->vNum; j++){
                    tm.push_back(__FLT_MAX__);
                    nm.push_back(-1);
                }
                tms.push_back(tm);
                nms.push_back(nm);
            }
            this->lowTimeCost[*iter] = tms;
            this->nextCrossForBest[*iter] = nms;
        }
    }

    void InitialMatrix()
    {
        for(auto iter = this->speedSet.begin(); iter != this->speedSet.end(); iter++){
            for(int i = 1; i <= this->vNum; i++){
                for(int j = 1; j <= this->vNum; j++){
                    this->lowTimeCost[*iter][i][j] = __FLT_MAX__;
                    this->nextCrossForBest[*iter][i][j] = -1;
                }
            }
        }
    }

    bool Check()
    {
        for(auto iter = this->speedSet.begin(); iter != this->speedSet.end(); iter++){
            // 对角线为__FLT_MAX，其它大于0
            vector<vector<float> > & timeMatrix = this->lowTimeCost[*iter];
            vector<vector<int> > & nextCrossMatrix = this->nextCrossForBest[*iter];
            for(int i = 1; i <= vNum; i++){
                for(int j = 1; j <= vNum; j++){
                    if(i==j){
                        if(timeMatrix[i][j] != __FLT_MAX__ || nextCrossMatrix[i][j] != -1)
                            return false;
                    }
                    /**else if(timeMatrix[i][j] < 0 || timeMatrix[i][j] > __FLT_MAX__)
                        return false;
                    else if(nextCrossMatrix[i][j] < -1 || nextCrossMatrix[i][j] > vNum)
                        return false; 暂且可以不要*/
                    else if(timeMatrix[i][j] == __FLT_MAX__ && nextCrossMatrix[i][j] != -1)
                        return false;
                    else if(timeMatrix[i][j] < __FLT_MAX__ && nextCrossMatrix[i][j] == -1)
                        return false;
                }
            }
        }
        return true;
    }

    void ComputeGraph(map<int, Road> & roadMap)
    {
        InitialMatrix();
        float velocity, t; // 临时变量
        for(auto sIter = this->speedSet.begin(); sIter != this->speedSet.end(); sIter++) // for each kind speed
        {
            int speed = *sIter;
            vector<vector<float> > & timeMatrix = this->lowTimeCost[speed];
            vector<vector<int> > & nextCrossMatrix = this->nextCrossForBest[speed];
            for(auto gIter = roadMap.begin(); gIter != roadMap.end(); gIter++)
            { // 初始化timeMatrix矩阵
                Road & road = gIter->second;
                Carport & carport = road.GetCarportStartFrom(road.from);
                velocity = speed < carport.flowRate*carport.speed ? speed : carport.flowRate*carport.speed; // 该边上的速度
                if(velocity == 0){
                    timeMatrix[crossOrder[road.from]][crossOrder[road.to]] = __FLT_MAX__;
                    nextCrossMatrix[crossOrder[road.from]][crossOrder[road.to]] = -1;
                }
                else{
                    t = carport.length / velocity;
                    if(t < 0)
                        getchar();
                    timeMatrix[crossOrder[road.from]][crossOrder[road.to]] = carport.length / velocity;
                    nextCrossMatrix[crossOrder[road.from]][crossOrder[road.to]] = road.to;
                }
                if(road.isDuplex == 1){
                    Carport & carport = road.GetCarportStartFrom(road.to);
                    velocity = speed < carport.flowRate*carport.speed ? speed : carport.flowRate*carport.speed; // 该边上的速度
                    if(velocity == 0){
                        timeMatrix[crossOrder[road.to]][crossOrder[road.from]] = __FLT_MAX__;
                        nextCrossMatrix[crossOrder[road.to]][crossOrder[road.from]] = -1;
                    }
                    else{
                        t = carport.length / velocity;
                        if(t < 0)
                            getchar();
                        timeMatrix[crossOrder[road.to]][crossOrder[road.from]] = carport.length / velocity;
                        nextCrossMatrix[crossOrder[road.to]][crossOrder[road.from]] = road.from;
                    }
                }
            }
            for(int k = 1; k <= vNum; k++)
            {
                for(int s = 1; s <= vNum; s++)
                {
                    for(int d = 1; d <= vNum; d++)
                    {
                        if(d == s || k == s || k == d)
                            continue;

                        t = timeMatrix[s][k] + timeMatrix[k][d];
                        if(t < 0)
                            getchar();
                        if(timeMatrix[s][d] > t) // 如果原来s到d没有路或者原来的路没有经过k的路好
                        {
                            timeMatrix[s][d] = t;
                            nextCrossMatrix[s][d] = nextCrossMatrix[s][k];
                        }
                    }
                }
            }
        }
    }

    void Show()
    {
        for(auto iter = network.begin(); iter != network.end(); iter++){
            int s = iter->first.first;
            int d = iter->first.second;
            int rid = iter->second;
            cout << s << ' ' << d << ' ' << rid << endl;
        }
        for(auto iter = this->speedSet.begin(); iter != this->speedSet.end(); iter++){
            vector<vector<float> > tim = this->lowTimeCost[(*iter)];
            for(int row = 1; row <= this->vNum; row++){
                for(int col = 1; col <= this->vNum; col++){
                    cout << tim[row][col] << ' ';
                }
                cout << endl;
            }
            vector<vector<int> > next = this->nextCrossForBest[(*iter)];
            for(int row = 1; row <= this->vNum; row++){
                for(int col = 1; col <= this->vNum; col++){
                    cout << next[row][col] << ' ';
                }
                cout << endl;
            }
        }
        cout << endl;
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

int notDestCar, notDestCarB;
int onRoadCar, onRoadCarB;

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
                        //WC.erase(car.id);
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
                            onRoadCar--;
                            notDestCar--;
                            waitCar--;
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
                    //WC.erase(car.id);
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
// 车辆方向lastDire全部设为-1
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
                car.lastDire = -1;
                lc_speed = (car.speed < carport.speed) ? car.speed : carport.speed;
                if(lc_speed > flag[1]-i-1){ //前方有阻挡
                    if(flag[0] == 0){ // 车辆阻挡，且阻挡车辆为终态，调整当前车辆位置并设置为终态
                        firstMoveStep += flag[1]-1-i;
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
int MoveFirWaitCar1(Car & car, Carport & fromCp, Carport & toCp, int dire, int & waitCar, int & stepNum)
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
                car.lastDire = dire;

                waitCar--;
                //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
                //WC.erase(car.id);
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
                    car.lastDire = dire;

                    waitCar--;
                    //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
                    //WC.erase(car.id);
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
    //WC.erase(car.id);
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
        car.lastDire = 1; // 到终点了都给设为直行吧

        waitCar--;
        onRoadCar--;
        notDestCar--;
        //cout << "car id = " << car.id << " on road " << fromCp.id << endl;
        //WC.erase(car.id);
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
        return MoveFirWaitCar1(firWaitCar, fromCp, roadMap[rid_left].GetCarportStartFrom(cross.id), 0, waitCar, stepNum);
        break;
    case 1: // 直行
        if(firWaitCar.order == firWaitCar.answer.size()-1)
            return MoveFirWaitCar2(firWaitCar, fromCp, waitCar, stepNum);
        else
            return MoveFirWaitCar1(firWaitCar, fromCp,roadMap[rid_ahead].GetCarportStartFrom(cross.id) , 1, waitCar, stepNum);
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
        return MoveFirWaitCar1(firWaitCar, fromCp, roadMap[rid_right].GetCarportStartFrom(cross.id), 2, waitCar, stepNum);
        break;
    }
    cout << "Error in MoveFirstWaitCar()" << endl;
    getchar();
    return -1;
}

// 这里不需要再判断前方车辆是何状态了，因为调用此函数前所有路上车辆都已经是终态
void DriveCarInGarage(int lc_time, int & dsTh, int & onRoadCar, vector<vector<int> > & cdo)
{
    bool flag = false; // 是否已经遇到了堵死的车辆
    for(int i = dsTh; i < cdo.size(); i++){
        if(cdo[i][1] > lc_time){
            if(flag == false) dsTh = i;
            return;
        }
        if(cdo[i][2] == 1)
            continue;
        Car & car = carMap[cdo[i][0]];
        Road & road = roadMap[car.answer[0]];
        //cout << "In DriveCarInGarage(): " << car.id << ' ' << road.id << ' ' << car.from << endl;

        Carport & carport = road.GetCarportStartFrom(car.from);
        int s = (car.speed<carport.speed) ? car.speed : carport.speed;
        int chn;
        for(chn = 0; chn < carport.chn; chn++){
            if(carport.tail[chn] > 0){
                s = ((s-1) < carport.tail[chn]) ? (s-1) : (carport.tail[chn]-1);
                carport.m[chn][s] = car.id;
                carport.tail[chn] = s;
                if(carport.head[chn] == -1) // 该车道上之前没有车辆
                    carport.head[chn] = s;
                car.chn = chn;
                car.order = 0;
                car.pos = s;
                car.status = 1;
                //car.dpStatus = 0; // 这里设不设终态无所谓，反正接下来马上要FirstAdjust()重新调整
                cdo[i][2] = 1; // 该车成功出发
                onRoadCar++;
                break;
            }
        }
        if(flag == false && chn == carport.chn){ // 该车未出发，被堵死
            dsTh = i;
            flag = true;
        }
    }
}
// 成功出发返回1，因拥堵需要延迟出发返回0
int DriveCarInGarageNew(Car & car, int space, int & thirdMoveStep)
{
    Road & road = roadMap[car.answer[0]];
    Carport & carport = road.GetCarportStartFrom(car.from);
    int lc_s = (car.speed<carport.speed) ? car.speed : carport.speed;
    for(int chn = 0; chn < carport.chn; chn++){
        if(carport.tail[chn] == 0)
            continue;
        if(carport.tail[chn] <= space) // 有空间但空间没有大于space要求
            return 0;
        lc_s = ((lc_s-1) < carport.tail[chn]) ? (lc_s-1) : (carport.tail[chn]-1);
        thirdMoveStep += lc_s + 1;
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
        onRoadCar++;
        //car.dpStatus = 0; // 这里设不设终态无所谓，反正接下来马上要FirstAdjust()重新调整
        return 1;
    }
    return 0;
}

int Judge(int& lc_time, int& allCarTime)
{
    return 1;
}
void Restore() /** carMap, roadMap*/
{
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        Car & car = iter->second;
        car.Restore();
    }
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        road.Restore();
    }
}

vector<int> GetRoadStartFrom(Cross & cross, int rid1, int rid2)
{
    vector<int> roads;
    for(int i = 0; i < cross.roadNum; i++){
        Road & road = roadMap[cross.roadIdAsc[i]];
        if(road.isDuplex == 1 || road.from == cross.id){
            if(road.id != rid1 && road.id != rid2) roads.push_back(road.id);
        }
    }
    return roads;
}
/**1. 随机走出前rd1步 + 随机叠加时间偏移
2. 测试一下是否所有车辆路线都能到终点
rd1 - 随机步数
rd2 - 时间偏移模*/
void CreateAnswerWithRandomFrontStep(vector<int> & carForDisp, vector<int> & carDelay, vector<int> & carReady, Graph & graph, int rd1)
{
    vector<int> roads;
    int cur, next;
    int rid = -1, i = 0;
    for(auto iter = carForDisp.begin(); iter != carForDisp.end(); iter++)
    {
        Car & car = carMap[*iter];
        if(car.answer.size() > 0){
            carReady.push_back(car.id);
            continue;
        }
        car.answer.clear();
        cur = car.from;
        while(rd1>0){
            i++;
            Cross & cross = crossMap[cur]; // 奇怪哦，为什么这样用就需要Cross()
            roads = GetRoadStartFrom(cross, rid, -1); // 前提是条条大路通罗马，如果这个前提不满足那么随机走几步就可能因为不能回头走而出不来
            if(roads.size()==0){
                cout << cross.id << ' ' << rid << "Why ????" << endl;
                getchar();
                break;
            }
            //srand((int)time(0));
            int t = rand()%roads.size();
            rid = roads[t];
            car.answer.push_back(rid);
            cur = (cur == roadMap[rid].from) ? roadMap[rid].to : roadMap[rid].from;

            if(i >= rd1){
                next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
                if(cur == car.to || (next != -1 && graph.network[CrossPair(cur,next)] != rid)){ // 随机出发几步之后不能到达终点，延迟
                    break;
                }
            }
        }
        if(rd1 == 0){
            next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
            if(next == -1){
                carDelay.push_back(car.id);
                continue;
            }
        }

        while(cur != car.to)
        {
            next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
            car.answer.push_back(graph.network[CrossPair(cur, next)]);
            cur = next;
        }
        carReady.push_back(car.id);
    }
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

// 更新道路拥堵参数
void UpdateRoadFlowrate(int theta){
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        Carport & carport = road.GetCarportStartFrom(road.from);
//        if(carport.num == 0) carport.flowRate = 1.0;
//        else carport.flowRate = (float)(carport.in+carport.out+theta)/(float)(carport.num+carport.out);
        carport.flowRate = (float)(carport.chn*carport.length)/(float)(carport.chn*carport.length+carport.num);
        carport.in = 0;
        carport.out = 0;
        //cout << carport.id << " " << carport.num << endl;
        if(road.isDuplex == 1){
            Carport & carport = road.GetCarportStartFrom(road.to);
            if(carport.num == 0) carport.flowRate = 1.0;
            else carport.flowRate = (float)(carport.in+carport.out+theta)/(float)(carport.num+carport.out);
            carport.in = 0;
            carport.out = 0;
            //cout << carport.id << " " << carport.num << endl;
        }
    }
}

void TurnFirCarTo(Car & firWaitCar, Cross & cross, Road & road_next, Graph & graph)
{ // 该函数只在MotifyFirCarDirection()中调用
    int cur, next, d = firWaitCar.to;
    int lc_size = firWaitCar.answer.size();
    for(int i = firWaitCar.order+1; i < lc_size; i++)
        firWaitCar.answer.pop_back();
    firWaitCar.answer.push_back(road_next.id); // 第一步去往road_next
    int lastRid = road_next.id;
    cur = (road_next.from == cross.id) ? road_next.to : road_next.from;
    while(cur != d){
        next = graph.nextCrossForBest[firWaitCar.speed][graph.crossOrder[cur]][graph.crossOrder[d]];
        if(next == -1 || graph.network[CrossPair(cur,next)] == lastRid){ // 随机去往一条路
            Cross & cross1 = crossMap[cur];
            vector<int> roads = GetRoadStartFrom(cross1, lastRid, -1);
            if(roads.size()==0)
                getchar();
            Road & road1 = roadMap[roads[rand()%roads.size()]];
            firWaitCar.answer.push_back(road1.id);

            cur = (road1.from == cross1.id) ? road1.to : road1.from;
            lastRid = road1.id;
        }
        else
            break;
    }
    while(cur != d){
        next = graph.nextCrossForBest[firWaitCar.speed][graph.crossOrder[cur]][graph.crossOrder[d]];
        firWaitCar.answer.push_back(graph.network[CrossPair(cur,next)]);
        cur = next;
    }
}

bool MotifyFirCarAnswer(Road & road, Cross & cross, Graph & graph)
{ // 调用该函数时第一等待车不可能即将到达终点

    if(GetRoadDirection(road, cross) == -1)
        return false;

    Carport & carport = road.GetCarportAheadTo(cross.id);
    Car & firWaitCar = carMap[carport.firWaitCarId];
    int rid1 = firWaitCar.answer[firWaitCar.order];
    int rid2 = firWaitCar.answer[firWaitCar.order+1];
    vector<int> roads = GetRoadStartFrom(cross, rid1, rid2);
    if(roads.size() == 0){
        return false;
    }
    else{
        int lc_rand = rand()%roads.size();
        Road & road_next = roadMap[roads[lc_rand]];
        TurnFirCarTo(firWaitCar, cross, road_next, graph);
        return true;
    }
}
bool ZuiJian(Road & road, Cross & cross, Graph & graph)
{
    if(GetRoadDirection(road, cross) == -1)
        return false;

    Carport & carport = road.GetCarportAheadTo(cross.id);
    Car & firWaitCar = carMap[carport.firWaitCarId];
    if(firWaitCar.breakpoint >= firWaitCar.order) // 实际上最多是等于
        return false;

    int rid1 = firWaitCar.answer[firWaitCar.order];
    int rid2 = firWaitCar.answer[firWaitCar.order+1];
    vector<int> roads = GetRoadStartFrom(cross, rid1, rid2);
    if(roads.size() == 0){
        return false;
    }

    int cur, next, d = firWaitCar.to;
    int rid_next = -1;
    int t = rand()%roads.size();
    for(int i = 0; i < roads.size(); i++){
        Road & road_next = roadMap[roads[(t+i)%roads.size()]];
        cur = (cross.id == road_next.from) ? road_next.to : road_next.from;
        next = graph.nextCrossForBest[firWaitCar.speed][graph.crossOrder[cur]][graph.crossOrder[d]];
        if(cur == d || (next != -1 && graph.network[CrossPair(cur,next)] != road_next.id)){
            rid_next = road_next.id;
            break;
        }
    }
    if(rid_next == -1)
        return false;

    int lc_size = firWaitCar.answer.size();
    for(int i = firWaitCar.order+1; i < lc_size; i++)
        firWaitCar.answer.pop_back();
    firWaitCar.breakpoint = firWaitCar.order;
    firWaitCar.answer.push_back(rid_next);

    while(cur != d){
        next = graph.nextCrossForBest[firWaitCar.speed][graph.crossOrder[cur]][graph.crossOrder[d]];
        firWaitCar.answer.push_back(graph.network[CrossPair(cur,next)]);
        cur = next;
    }
    return true;
}

void ZuiJian2(Road & road, Graph & graph)
{
    Carport & carport = road.carport1;
    for(int chn = 0; chn < carport.chn; chn++){
        for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
            if(carport.m[chn][i] != -1){
                Car & car = carMap[carport.m[chn][i]];
                if(car.breakpoint >= car.order) // 实际上最多是等于
                    continue;
                if(car.order == car.answer.size()-1)
                    continue;
                int cid = graph.network1[CrossPair(car.answer[car.order],car.answer[car.order+1])];
                Cross & cross = crossMap[cid];
                int cur = cross.id;
                int next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
                if(next != -1 && graph.network[CrossPair(cur,next)] != car.answer[car.order]){
                    int lc_size = car.answer.size();
                    for(int j = car.order+1; j < lc_size; j++)
                        car.answer.pop_back();
                    car.breakpoint = car.order;
                    while(cur != car.to){
                        next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
                        car.answer.push_back(graph.network[CrossPair(cur,next)]);
                        cur = next;
                    }
                }
            }
        }
    }
    if(road.isDuplex == 1){
        Carport & carport = road.carport2;
        for(int chn = 0; chn < carport.chn; chn++){
            for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
                if(carport.m[chn][i] != -1){
                    Car & car = carMap[carport.m[chn][i]];
                    if(car.breakpoint >= car.order) // 实际上最多是等于
                        continue;
                    if(car.order == car.answer.size()-1)
                        continue;
                    int cid = graph.network1[CrossPair(car.answer[car.order],car.answer[car.order+1])];
                    Cross & cross = crossMap[cid];
                    int cur = cross.id;
                    int next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
                    if(next != -1 && graph.network[CrossPair(cur,next)] != car.answer[car.order]){
                        int lc_size = car.answer.size();
                        for(int j = car.order+1; j < lc_size; j++)
                            car.answer.pop_back();
                        while(cur != car.to){
                            next = graph.nextCrossForBest[car.speed][graph.crossOrder[cur]][graph.crossOrder[car.to]];
                            car.answer.push_back(graph.network[CrossPair(cur,next)]);
                            cur = next;
                        }
                    }
                }
            }
        }
    }
}
// 将该道路第一车辆改为右转，如果本来就是要想右转，那么有如下两个选择
// 1. 如果左转道路尾部没有之前直行过路口的车辆，那么左转
// 2. 如果直行道路尾部没有之前转向过路口的车辆，那么直行
bool MotifyFirCarDirection(Road & road, Cross & cross, Graph & graph)
{ // 调用该函数时第一等待车不可能即将到达终点
    int dire = GetRoadDirection(road, cross);
    if(dire == -1)
        return false;

    Carport & carport = road.GetCarportAheadTo(cross.id);
    Car & firWaitCar = carMap[carport.firWaitCarId];
    int rid = cross.locRelaInfo[road.id][2];

//    if(firWaitCar.answer[firWaitCar.order+1] != rid){ // 车辆不是因为右转死锁
//        if(rid != -1){
//            Road & road_next = roadMap[rid];
//            if(road_next.from == cross.id || road_next.isDuplex == 1){
//                TurnFirCarTo(firWaitCar, cross, road_next, graph);
//                return true;
//            }
//        }
//    }

    if(firWaitCar.answer[firWaitCar.order+1] == rid){ // 车辆之前是右转

    bool canLeft = true, canAhead = true; // 能否左转或直行

    rid = cross.locRelaInfo[road.id][0]; // 左转道路
    if(rid != -1){
        Road & road_next = roadMap[rid];
        if(road_next.isDuplex == 1 || road_next.from == cross.id){
            Carport & carport = road_next.GetCarportStartFrom(cross.id);
            for(int chn = 0; chn < carport.chn; chn++){
                for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
                    if(carport.m[chn][i] != -1){
                        Car & car = carMap[carport.m[chn][i]]; // 前方某车道尾部车辆
                        if(car.lastDire == 2){ // 前方车辆上次转向为右转，因此不能选择左转
                            canLeft = false;
                            break;
                        }
                        else if(car.lastDire == -1){ // 该车道不需要再往前判断
                            break;
                        }
                    }
                }
                if(!canLeft) // 已经不可能，不需要再判断其它车道车辆
                    break;
            }
            if(canLeft){
                TurnFirCarTo(firWaitCar, cross, road_next, graph);
                return true;
            }
        }
    }

    // 如果不能左转，继续判断能否直行
    rid = cross.locRelaInfo[road.id][1];
    if(rid != -1){
        Road & road_next = roadMap[rid];
        if(road_next.isDuplex == 1 || road_next.from == cross.id){
            Carport & carport = road_next.GetCarportStartFrom(cross.id);
            for(int chn = 0; chn < carport.chn; chn++){
                for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
                    if(carport.m[chn][i] != -1){
                        Car & car = carMap[carport.m[chn][i]]; // 前方某车道尾部车辆
                        if(car.lastDire == 0){ // 前方车辆上次转向为左转，因此不能选择直行
                            canAhead = false;
                            break;
                        }
                        else if(car.lastDire == 2){ // 前方车辆上次转向为右转，因此不能选择直行
                            canAhead = false;
                            break;
                        }
                        else if(car.lastDire == -1){// 该车道不需要再往前判断
                            break;
                        }
                    }
                }
                if(!canAhead) // 已经不可能，不需要再判断其它车道车辆
                    break;
            }
            if(canAhead){
                TurnFirCarTo(firWaitCar, cross, road_next, graph);
                return true;
            }
        }
    }
    }
    return false;
}
void Debug()
{
    bool flag = false; // finded -1?
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        Carport & carport = road.carport1;
        for(int chn = 0; chn < carport.chn; chn++){
            flag = false;
            for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
                if(carport.m[chn][i] != -1){
                    Car& car = carMap[carport.m[chn][i]];
                    if(flag && car.lastDire > -1){
                        carport.Show();
                        getchar();
                    }
                    if(car.lastDire == -1)
                        flag = true;
                }
            }
        }
        if(road.isDuplex == 1){
            Carport & carport = road.carport1;
            for(int chn = 0; chn < carport.chn; chn++){
                flag = false;
                for(int i = carport.tail[chn]; i <= carport.head[chn]; i++){
                    if(carport.m[chn][i] != -1){
                        Car& car = carMap[carport.m[chn][i]];
                        if(flag && car.lastDire > -1){
                            carport.Show();
                            getchar();
                        }
                        if(car.lastDire == -1)
                            flag = true;
                    }
                }
            }
        }
    }
}
void AllVarFromBackup()
{
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        Car & car = iter->second;
        car.FromBackup();
    }
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        road.carport1.FromBackup();
        if(road.isDuplex == 1)
            road.carport2.FromBackup();
    }
}
void AllVarToBackup()
{
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        Car & car = iter->second;
        car.Backup();
    }
    for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++){
        Road & road = iter->second;
        road.carport1.Backup();
        if(road.isDuplex == 1)
            road.carport2.Backup();
    }
}

int Simulation(struct THETA & theta, Graph & graph, int & lc_time, int & allCarTime)
{
    /**新方法：速度大的先走，定期更新最短路矩阵**/
    lc_time = 0;
    allCarTime = 0;
    int firstMoveStep, secondMoveStep, thirdMoveStep;
    vector<vector<int> > carSet; // carid-asc, planTime-asc
    int lastIdx = 0, lastIdxB; // carSet中上一次访问到的位置
    vector<int> carForDisp, carForDispB; // 当前时间片待调度的车辆

    // 首先构造车辆总的初步调度顺序
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
        vector<int> temp;
        temp.push_back(iter->second.id);
        temp.push_back(iter->second.planTime);
        carSet.push_back(temp);
    }
    sort(carSet.begin(),carSet.end(),comp);

    notDestCar = carSet.size();
    onRoadCar = 0;
    int timeB, onRoadCarB,notDestCarB,allCarTimeB;
    bool DeadLock = false;
    // 进入调度模拟
    while(notDestCar > 0){
        lc_time++;
        allCarTime += onRoadCar;

        //cout << lc_time << " " << notDestCar << " " <<  carMap.size() << " " << crossMap.size() << " " << roadMap.size() << " " << graph.network.size() << " " << graph.network1.size() << endl;

        /**第一步：驱动路上所有能到达终态的车辆到达终态，并标记车辆状态，所有车辆要么标记为等待态，要么终态*/
        int waitCar = 0, old; // 等待的车辆数
        firstMoveStep = 0;
        //WC.clear();
        for(auto iter = roadMap.begin(); iter != roadMap.end(); iter++)
        {// 这个道路调度的先后没有关系
            Road & road = iter->second;
            FirstAdjust(road.carport1, waitCar, firstMoveStep);
            if(road.isDuplex == 1)
                FirstAdjust(road.carport2, waitCar, firstMoveStep);
        }

        /**第二步：处理等待的车辆*/
        secondMoveStep = 0;
        while(waitCar > 0){ // 虽然等待车辆全部设为终态，但是可能因为堵塞没有产生任何移动，怎么解决？
            secondMoveStep = 0;
            old = waitCar;
            for(auto iter = crossMap.begin(); iter != crossMap.end(); iter++)
            { // 按路口编号从大到小进行调度
                Cross & cross = iter->second;
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
                cout << "dead lock" << endl;
                cout << "lc_time = " << lc_time << ", allCarTime = " << allCarTime << ", waitCar = " << waitCar << endl;
                // 修改路径，使得所有道路第一车辆在下一个循环都能达到终态
                //Debug();
               DeadLock = true;
               UpdateRoadFlowrate(theta.flow);
               graph.ComputeGraph(roadMap);
               for(auto iter = crossMap.begin(); iter != crossMap.end(); iter++){
                   Cross & cross = iter->second;
                   for(int i = 0; i < cross.roadNum; i++){
                       Road & road = roadMap[cross.roadIdAsc[i]];
                       ZuiJian(road, cross, graph);
                       if(rand()%5 == 1)
                           ZuiJian2(road, graph);
                   }
               }
               break;
            }
        }
        //cout << WC.size() << " " << waitCar << endl;
        /**第三步，驱动车库车辆上路*/
        if(DeadLock){
            cout << "restore info from lc_time " << timeB << endl;
            AllVarFromBackup();
            onRoadCar = onRoadCarB;
            notDestCar = notDestCarB;
            lc_time = timeB;
            allCarTime = allCarTimeB;
            lastIdx = lastIdxB;
            carForDisp = carForDispB;
            DeadLock = false;
        }
        else if(lc_time%theta.backupT == 0){
            cout << "create backup file at lc_time " << lc_time << endl;
            AllVarToBackup();
            onRoadCarB = onRoadCar;
            notDestCarB = notDestCar;
            timeB = lc_time;
            allCarTimeB = allCarTime;
            lastIdxB = lastIdx;
            carForDispB = carForDisp;
        }

        int afterSecondStep = onRoadCar; // 经过该时间片调度后还在路上的车辆
        thirdMoveStep = 0;
        // 取出当前时间片车辆
        while(lastIdx < carSet.size() && carSet[lastIdx][1] == lc_time){
            carForDisp.push_back(carSet[lastIdx][0]);
            lastIdx++;
        }
        sort(carForDisp.begin(), carForDisp.end());

        if(onRoadCar > theta.volumn){
            for(auto iter = carForDisp.begin(); iter != carForDisp.end(); iter++){
                carMap[*iter].realTime++;
            }
        }
        else{
        // 为当前时间片的所有车辆计算规划路径
        vector<int> carDelay; // 将要延迟的车辆
        vector<int> carReady; // 能得到出发路线的车辆
        CreateAnswerWithRandomFrontStep(carForDisp, carDelay, carReady, graph, theta.randomstep);
        carForDisp.clear();

        // 将车辆下发到车库做发车准备
        map<pair<int,int>, vector<vector<int> > > garage; // (crossid,roadid) - [[carid, v]]
        for(auto iter = carReady.begin(); iter != carReady.end(); iter++){ // 这个操作之后carid默认是有序的
            Car & car = carMap[(*iter)];
            vector<vector<int> > & garg = garage[pair<int,int>(car.from, car.answer[0])];
            if(garg.size() == 0 || car.speed == garg[0][1]){
                vector<int> temp;
                temp.push_back(car.id);
                temp.push_back(car.speed);
                garg.push_back(temp);
            }
            else if(car.speed > garg[0][1]){
                for(int i = 0; i < garg.size(); i++){
                    carDelay.push_back(garg[i][0]);
                }
                garg.clear();
                vector<int> temp;
                temp.push_back(car.id);
                temp.push_back(car.speed);
                garg.push_back(temp);
            }
            else{
                carDelay.push_back(car.id);
            }
        }

        // 调度车库车辆上路
        for(auto iter = garage.begin(); iter != garage.end(); iter++){
            vector<vector<int> > & garg = iter->second;
            for(int i = 0; i < garg.size(); i++){
                Car & car = carMap[garg[i][0]];
                if(DriveCarInGarageNew(car, theta.space, thirdMoveStep)==0){// 拥堵，延迟发车
                    for(int j = i; j < garg.size(); j++){
                        carDelay.push_back(garg[j][0]);
                    }
                    break;
                }
            }
            garg.clear();
        }

        // 更改延迟车辆的实际出发时间并保存到待调度列表
        for(auto iter = carDelay.begin(); iter != carDelay.end(); iter++){
            carMap[(*iter)].realTime++;
            carForDisp.push_back((*iter));
        }
        }

        allCarTime += onRoadCar - afterSecondStep; // 该时间片新上路的车辆数

        if(firstMoveStep+secondMoveStep+thirdMoveStep == 0){
            cout << "yong du, please modify theta_space and Simulation() again!" << endl;
            return -1;
        }

        if(lc_time%theta.T == 0){ // 定期更新道路图
            // 一次循环之后更新道路流动指数
            cout << lc_time<< " " << notDestCar << endl;
            UpdateRoadFlowrate(theta.flow);
            graph.ComputeGraph(roadMap);
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    cout << "Begin" << endl;
    string carPath("../1-map-exam-1/car.txt");
    string roadPath("../1-map-exam-1/road.txt");
    string crossPath("../1-map-exam-1/cross.txt");
    string answerPath("../1-map-exam-1/answer.txt");

    cout << "carPath is " << carPath << endl;
    cout << "roadPath is " << roadPath << endl;
    cout << "crossPath is " << crossPath << endl;
    cout << "answerPath is " << answerPath << endl;

    Txt2Map(carPath, carMap);
    Txt2Map(roadPath, roadMap);
    Txt2Map(crossPath, crossMap);

    Graph graph(roadMap, crossMap);
    srand((int)time(0));

    int lc_time, allCarTime;
    int timeBest = __INT_MAX__, allCarTimeBest = __INT_MAX__;
    struct THETA theta;
    theta.volumn = 4000;
    theta.randomstep = 0;
    theta.T = 10;
    theta.space = 3;

    for(int i = 0; i < 1; i++){
        Restore();
        graph.ComputeGraph(roadMap);
        int r = Simulation(theta, graph, lc_time, allCarTime);
        if(r == 0){ // 死锁解决不了
            theta.volumn -= rand()%50;
            theta.randomstep += rand()%2;
        }
        else if(r == -1){ // 拥堵
            theta.volumn -= rand()%50;
            theta.space += rand()%2;
        }
        else{ // r == 1 仿真成功，判断结果是否比上次好
            if((lc_time < timeBest) || (lc_time == timeBest && allCarTime < allCarTimeBest)){
                timeBest = lc_time;
                allCarTimeBest = allCarTime;
                for(auto iter = carMap.begin(); iter != carMap.end(); iter++){
                    iter->second.UpdateAnswer();
                }
                cout << "Get a better answer!" << endl;
            }
            cout << "lc_time: " << lc_time << ", allCarTime: " << allCarTime << endl;
            cout << "volumn: " << theta.volumn << endl;
            cout << "space: " << theta.space << endl;
            cout << "randomstep: " << theta.randomstep << endl;
            cout << "flow: " << theta.flow << endl;
            cout << "T: " << theta.T << endl;

            theta.volumn -= rand()%50;
        }
    }

    cout << "lc_time = " << timeBest << ", allCarTime = " << allCarTimeBest<< endl;

    // 将结果写入文件
    ofstream fout;
    fout.open(answerPath, ios_base::out);
    string line = "#(carId, startTime, roadId, ... )\n";
    fout << line;
    for(auto iter = carMap.begin(); iter != carMap.end(); iter++)
    {
        Car & car = iter->second;
        line = "("+to_string(car.id)+", "+to_string(car.realTimeBackup)+", ";
        for(auto iter = car.answerBackup.begin(); iter != car.answerBackup.end(); iter++)
        {
            line += to_string(*iter) + ", ";
        }
        line[line.length()-2] = ')';
        line[line.length()-1] = '\n';
        fout << line;
    }
    fout.close();
    cout << "write complete in " << answerPath << endl;

    return 0;
}
