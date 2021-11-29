#define PORT 8001
#define PASSLEN 20
#define NAMELEN 20
#define tname_LEN 20
#define CONCUR 10
#define DEF_SEATS 100
#define IP "127.0.0.1"

struct train{
		int train_id;
		char train_name[tname_LEN];
		int capacity;
		int available;
		int deleted;
		};
struct user{
		int user_id;
		char pswd[PASSLEN];
		char name[NAMELEN];
		int type;
		int deleted;
		};

struct booking{
		int booking_id;
		int type;
		int uid;
		int train_id;
		int seats;
		int deleted;
		};

