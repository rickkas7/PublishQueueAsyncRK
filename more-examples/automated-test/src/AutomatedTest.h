#ifndef __AUTOMATEDTEST_H
#define __AUTOMATEDTEST_H

#include "PublishQueueAsyncRK.h"
#include "SerialCommandParserRK.h"



class AutomatedTest {
public:
    void setup(PublishQueueAsyncBase *publishQueue);

    void loop();

    void publishPaddedCounter(int size, bool withAck);

    void resetSettings();


    PublishQueueAsyncBase *publishQueue;
    SerialCommandParser<1000, 16> commandParser;
    bool doReset = false;
    unsigned long exitTime = 0;

    // Publisher
    int counter = 0;
    int count = 0;
    String data;
    String name = "testEvent";
    unsigned long period = 0;
    int size = 0;

    // State
    int numPublished = 0;
    unsigned long lastPublish = 0;

};



#endif /* __AUTOMATEDTEST_H */



