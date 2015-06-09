
struct TideEvent_s
{
	int timestamp;
	int height_100;
	int is_high_tide; 
};


typedef union TideEvent
{
	struct TideEvent_s event;
	char bytes[12];
} TideEvent;