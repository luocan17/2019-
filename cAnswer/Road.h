#ifndef ROAD_H
#define ROAD_H

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

    static int count;

public:
    Road(){}

    Road(int info[])
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

    Road(const Road& road)
    {
        this->id = road.id;
        this->length = road.length;
        this->speed = road.speed;
        this->channel = road.channel;
        this->from = road.from;
        this->to = road.to;
        this->isDuplex = road.isDuplex;
    }

    Road Mirror()
    {
        Road mirrorRoad(*this);
        mirrorRoad.from = this->to;
        mirrorRoad.to = this->from;
        return mirrorRoad;
    }
};

#endif // ROAD_H
