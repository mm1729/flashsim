#include "ssd.h"


using namespace ssd;


Request::Request(Event* event, FtlParent* ftl)
{
    this->event = event;
    this->ftl = ftl;
}

void Request::setFtlImpl(FtlParent* ftl)
{
    this->ftl = ftl;
}

void Request::setValue(Event* event)
{
    this->event = event;
} 

void Request::process()
{
    printf("here in process\n");
    if(this->event->get_event_type() == READ)
        this->ftl->read(*this->event);
    else if(this->event->get_event_type() == WRITE)
        this->ftl->write(*this->event);
    else if(this->event->get_event_type() == TRIM)
        this->ftl->trim(*this->event);
    else
        fprintf(stderr, "Controller: %s: Invalid event type\n", __func__);
    
}