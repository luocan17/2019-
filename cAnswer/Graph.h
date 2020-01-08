#ifndef GRAPH_H
#define GRAPH_H

#include "Road.h"
#include "Cross.h"
#include "Car.h"
#include <map>
#include <iostream>
#include <fstream>
#include <utility>
#include <cstring>
#include <vector>
#include <set>

typedef std::pair<int, int> CrossPair;

template<class _T> class Matrix2D
{
public:
    _T **m = NULL;
    int r, c;
    Matrix2D() {}

    Matrix2D(unsigned int row, unsigned int col)
    {
        this->r = row;
        this->c = col;

        this->m = new _T *[row];
        for(int i = 0; i < row; i++)
        {
            this->m[i] = new _T[col];
            memset(this->m[i], -1, col*sizeof(_T));
        }
    }

    void Show()
    {
        for(int i = 0; i < this->r; i++)
        {
            for(int j = 0; j< this->c; j++)
                std::cout << this->m[i][j] << ' ';
            std::cout << std::endl;
        }
    }
};

class Graph
{
public:
    std::map<CrossPair, Road> roadMap;

    int vNum; // 顶点数量

    std::map<int, Matrix2D<int> > lowTimeCost; // 不同车速在道路网中，任意两地之间的最少用时
    std::map<int, Matrix2D<int> > nextCrossForBest; // 不同车速在道路网中，从p到q最优路线时下一个要到达的路口

    Graph(const std::vector<Road> & roadSet)
    {
        // 构造图
        for(auto iter = roadSet.begin(); iter != roadSet.end(); iter++)
        {
            Road road(*iter);
            this->roadMap.insert(std::make_pair(CrossPair(road.from, road.to), road));
            if(road.isDuplex == 1)
            {
                Road road1 = road.Mirror();
                this->roadMap.insert(make_pair(CrossPair(road1.from, road1.to), road1));
            }
        }

        vNum = Cross::count;

    }

    void ComputeGraph()
    {
        for(auto sIter = Car::speedSet.begin(); sIter != Car::speedSet.end(); sIter++) // for each kind speed
        {
            int speed = *sIter;
            Matrix2D<int> timeMatrix(vNum+1, vNum+1);
            Matrix2D<int> nextCrossMatrix(vNum+1, vNum+1);
            //timeMatrix.Show();
            //nextCrossMatrix.Show();

            for(auto gIter = roadMap.begin(); gIter != roadMap.end(); gIter++)
            { // 初始化timeMatrix矩阵
                auto crossPair = gIter->first;
                int velocity = speed < gIter->second.speed ? speed : gIter->second.speed; // 该边上的速度
                int t = int(gIter->second.length / float(velocity) + 0.5);
                timeMatrix.m[crossPair.first][crossPair.second] = t;
                nextCrossMatrix.m[crossPair.first][crossPair.second] = crossPair.second;
            }
            for(int k = 1; k <= vNum; k++)
            {
                for(int s = 1; s <= vNum; s++)
                {
                    for(int d = 1; d <= vNum; d++)
                    {
                        if(d == s)
                            continue;
                        if(timeMatrix.m[s][k] != -1 && timeMatrix.m[k][d] != -1) // 如果k的加入能增加一条从s到d的通路
                        {
                            int t = timeMatrix.m[s][k] + timeMatrix.m[k][d];
                            if(timeMatrix.m[s][d] == -1 || timeMatrix.m[s][d] > t) // 如果原来s到d没有路或者原来的路没有经过k的路好
                            {
                                timeMatrix.m[s][d] = t;
                                nextCrossMatrix.m[s][d] = nextCrossMatrix.m[s][k];
                            }
                        }
                    }
                }
            }
            //this->lowTimeCost[speed] = timeMatrix;
            this->lowTimeCost.insert(std::make_pair(speed, timeMatrix));
            this->nextCrossForBest.insert(std::make_pair(speed, nextCrossMatrix));
            //this->lowTimeCost[speed].Show(); // 为什么准备用它了就要求Matrix2D()，map的机制到底是什么样的??
            //this->nextCrossForBest[speed].Show();
        }
    }
};

#endif // GRAPH_H
