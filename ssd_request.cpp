#include "ssd.h"


using namespace ssd;


Request::Request(Event &event, FtlParent* ftl) : event(event)
{
    //this->event = event;
    this->ftl = ftl;
}

void Request::setFtlImpl(FtlParent* ftl)
{
    this->ftl = ftl;
}

void Request::setValue(Event &event)
{
    this->event = event;
} 

void Request::process()
{
    //printf("here in process\n");
    /*if(event.get_logical_address() != 10) {
		fprintf(stderr, "In func: %s: address %ud \n", __func__, event.get_logical_address());
	}*/
    if(this->event.get_event_type() == READ)
        this->ftl->read_(this->event);
    else if(this->event.get_event_type() == WRITE)
        this->ftl->write_(this->event);
    else if(this->event.get_event_type() == TRIM)
        this->ftl->trim(this->event);
    
}

Event Request::getValue() {
    return this->event;
}