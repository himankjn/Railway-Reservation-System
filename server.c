
#include "common.h"
#include "conf.h"

//change the port_no, name length, password length in common.h file, if needed.

void service_req(int client_fd);
void login(int client_sd);
void signup(int client_sd);
void post_login_ops(int client_sd,int type,int id);
void train_ops(int client_sd);
void user_ops(int client_sd);
void booking_operation(int client_sd,int choice,int type,int id);
   
int main(void) {
 
    struct sockaddr_in server, client; 
  
    int sd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sd == -1) { 
        printf("Could not create socket"); 
    } 
    
    server.sin_family = AF_INET; 
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT); 
    
    int x=bind(sd, (struct sockaddr*)&server, sizeof(server));
    if(x==-1)printf("Error binding");
    // how many concurrent requests can be handled.
    x=listen(sd, CONCUR);  
    if(x==-1)printf("Error binding");
    
    printf("Server running and waiting for client requests\n");
    while (1){
	    int client_sd = accept(sd, (struct sockaddr*)NULL,NULL);
	    if (!fork()){
		    close(sd);
		    service_req(client_sd);	
		    //exit after servicing the client.	
		    exit(0);
	    }
    }
    return 0;
}

// Every client is redirected here after connection establishment.
void service_req(int client_fd){
	int choice;
	printf("Client Connected: %d\n", client_fd);
	while(1){
		read(client_fd, &choice, sizeof(int));		
		if(choice==1)
			login(client_fd);
		else if(choice==2)
			signup(client_fd);
		else if(choice==3)
			break;
	}

	close(client_fd);
	printf("Client Disconnected: %d", client_fd);
}

//login window
void login(int client_sd){

	//read id and pswd from user for authetication
	int id;
	char pswd[PASSLEN];
	read(client_sd,&id,sizeof(id));
	read(client_sd,&pswd,sizeof(pswd));
	
	//lock the record of user file where user_id=id
	int user_fd = open("database/user",O_RDWR);
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = (id-1)*sizeof(struct user);
	lock.l_len = sizeof(struct user);
	lock.l_pid = getpid();
	fcntl(user_fd,F_SETLKW, &lock);
	
	//verify user credentials.
	struct user u;
	int legit=0,type;
	
	while(read(user_fd,&u,sizeof(u))){
		if(u.user_id==id && strcmp(u.pswd,pswd)==0){
				legit=1;
				type = u.type;
				break;
			
		}		
	}
	
	// if type of account is 'AGENT', unlock to allow many agents.
	if(type==2){
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
	}
	
	// if user is authenticated successfully allow further operations.
	if(legit)
	{
			write(client_sd,&legit,sizeof(legit));
			write(client_sd,&type,sizeof(type));
			post_login_ops(client_sd,type,id);

	}
	else
		write(client_sd,&legit,sizeof(legit));
	
	//unlocking customer and admin user locks after servicing their req.
	if(type==1 || type==3){
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
	}
} 

void post_login_ops(int client_sd,int type,int id){
	int choice;
	
	//admin
	if(type==1){
		read(client_sd,&choice,sizeof(choice));
		if(choice==1){					
			train_ops(client_sd);
			post_login_ops(client_sd,type,id);	
		}
		else if(choice==2){				
			user_ops(client_sd);
			post_login_ops(client_sd,type,id);	
		}
		else if (choice ==3)				
			return;
	}
	
	//customer or agent
	else if(type==2 || type==3){				
		read(client_sd,&choice,sizeof(choice));
		booking_operation(client_sd,choice,type,id);
		if(choice!=5)
			post_login_ops(client_sd,type,id);
	}		
}

void booking_operation(int client_sd,int choice,int type,int id){
	//booking
	if(choice==1){	
		//show trains to user for booking. (the client explicitly sends the view choice to train_ops fnc)
		train_ops(client_sd);
		//lock the train which needs to be booked 
		int train_id;
		read(client_sd,&train_id,sizeof(train_id));
		
		int fd_train = open("database/train", O_RDWR);
		struct flock lock_train;
		lock_train.l_type = F_WRLCK;
		lock_train.l_whence = SEEK_SET;
		lock_train.l_start = train_id*sizeof(struct train);
		lock_train.l_len = sizeof(struct train);
		lock_train.l_pid = getpid();
		
		fcntl(fd_train, F_SETLKW, &lock_train);
		lseek(fd_train,train_id*sizeof(struct train),SEEK_SET);
		
		//lock the booking file for consistent database
		int fd_book = open("database/booking", O_RDWR);

		struct flock lock_booking;
		lock_booking.l_type = F_WRLCK;
		lock_booking.l_start = 0;
		lock_booking.l_len = 0;
		lock_booking.l_whence = SEEK_END;
		lock_booking.l_pid = getpid();
		
		//verify train id and verify available seats> required seats. if so lock booking and book the tickets.
		struct train record_t;
		struct booking record_b;
		int req_seats;
		read(fd_train,&record_t,sizeof(record_t));
		//read number of seats required from user.
		read(client_sd,&req_seats,sizeof(req_seats));

		int valid=0;
		if(record_t.train_id==train_id)
		{		
			if(record_t.available >= req_seats){
				valid = 1;
				record_t.available -= req_seats;
				fcntl(fd_book, F_SETLKW, &lock_booking);
				int fp = lseek(fd_book, 0, SEEK_END);
				
				if(fp > 0){
					lseek(fd_book, -1*sizeof(struct booking), SEEK_CUR);
					read(fd_book, &record_b, sizeof(struct booking));
					record_b.booking_id++;
				}
				else 
					record_b.booking_id = 0;

				record_b.type = type;
				record_b.deleted=0;
				record_b.uid = id;
				record_b.train_id = train_id;
				record_b.seats = req_seats;
				write(fd_book, &record_b, sizeof(struct booking));
				lock_booking.l_type = F_UNLCK;
				fcntl(fd_book, F_SETLK, &lock_booking);
			 	close(fd_book);
			}
		
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		write(fd_train, &record_t, sizeof(record_t));
		}

		//unlock the train database
		lock_train.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock_train);
		close(fd_train);
		//inform client if operation was successful.
		write(client_sd,&valid,sizeof(valid));
			
	}
	
	
	//View (show user his bookings)
	else if(choice==2){		
	
		//lock the booking database using read lock.			
		struct flock lock_booking;
		
		lock_booking.l_type = F_RDLCK;
		lock_booking.l_whence = SEEK_SET;
		lock_booking.l_start = 0;
		lock_booking.l_len = 0;
		lock_booking.l_pid = getpid();
		int fd_book = open("database/booking", O_RDONLY);
		fcntl(fd_book, F_SETLKW, &lock_booking);
		
		struct booking record_b;
		int total_bookings = 0;
		while(read(fd_book,&record_b,sizeof(record_b))){
			if (record_b.uid==id)
				total_bookings++;
		}
		
		write(client_sd, &total_bookings, sizeof(int));
		lseek(fd_book,0,SEEK_SET);

		while(read(fd_book,&record_b,sizeof(record_b))){
			if(record_b.uid==id){
				write(client_sd,&record_b.booking_id,sizeof(int));
				write(client_sd,&record_b.train_id,sizeof(int));
				write(client_sd,&record_b.seats,sizeof(int));
			}
		}
		
		//unlock the booking database.
		lock_booking.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lock_booking);
		close(fd_book);
	}

	//update
	else if (choice==3){						
		//show user his bookings
		booking_operation(client_sd,2,type,id);
		
		//read which booking is to be updated
		int booking_id;
		read(client_sd,&booking_id,sizeof(booking_id));
		
		//lock booking database
		struct flock lock_booking;
		int fd_book = open("database/booking", O_RDWR);
		lock_booking.l_type = F_WRLCK;
		lock_booking.l_start = booking_id*sizeof(struct booking);
		lock_booking.l_len = sizeof(struct booking);
		lock_booking.l_whence = SEEK_SET;
		lock_booking.l_pid = getpid();
		fcntl(fd_book, F_SETLKW, &lock_booking);
		
		//read the required booking record
		lseek(fd_book,booking_id*sizeof(struct booking),SEEK_SET);
		struct booking record_b;
		read(fd_book,&record_b,sizeof(record_b));
		
		//lock the target train record in train database.
		struct flock lock_train;
		
		lock_train.l_type = F_WRLCK;
		lock_train.l_whence = SEEK_SET;
		lock_train.l_start = (record_b.train_id)*sizeof(struct train);
		lock_train.l_len = sizeof(struct train);
		lock_train.l_pid = getpid();
		
		int fd_train = open("database/train", O_RDWR);
		fcntl(fd_train, F_SETLKW, &lock_train);
		
		//read the target train record.
		lseek(fd_train,(record_b.train_id)*sizeof(struct train),SEEK_SET);
		struct train record_t;
		read(fd_train,&record_t,sizeof(record_t));

		
		int change;
		int valid=0;
		//modify booked seats						
		read(client_sd,&change,sizeof(choice));
		if(change>=0 && record_t.available >= change || change<0 && abs(change)<=record_b.seats){
			valid=1;
			record_t.available -= change;
			record_b.seats += change;
		}
		
		
		//write updated train and booking record to database & unlock them
		lseek(fd_train,-1*sizeof(struct train),SEEK_CUR);
		write(fd_train,&record_t,sizeof(record_t));
		lock_train.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock_train);
		close(fd_train);
		
		lseek(fd_book,-1*sizeof(struct booking),SEEK_CUR);
		write(fd_book,&record_b,sizeof(record_b));
		lock_booking.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lock_booking);
		close(fd_book);
		
		//update client if update was successful
		write(client_sd,&valid,sizeof(valid));
		
	}
	
	//cancel booking
	else if(choice==4){
		int valid=0;
		//show the bookings to user
		booking_operation(client_sd,2,type,id);
		
		//read the booking id to cancel from client
		int booking_id;
		read(client_sd,&booking_id,sizeof(booking_id));
		
		//lock the relevant booking record
		struct flock lock_booking;
		lock_booking.l_whence = SEEK_SET;
		lock_booking.l_type = F_WRLCK;
		lock_booking.l_start = booking_id*sizeof(struct booking);
		lock_booking.l_len = sizeof(struct booking);
		lock_booking.l_pid = getpid();
		int fd_book = open("database/booking", O_RDWR);
		fcntl(fd_book, F_SETLKW, &lock_booking);
		
		//read the relevant booking record.
		struct booking record_b;
		lseek(fd_book,booking_id*sizeof(struct booking),SEEK_SET);
		read(fd_book,&record_b,sizeof(record_b));
		
		//lock the corresponding train record of the booking.
		struct flock lock_train;
		lock_train.l_type = F_WRLCK;
		lock_train.l_whence = SEEK_SET;
		lock_train.l_start = (record_b.train_id)*sizeof(struct train);
		lock_train.l_len = sizeof(struct train);
		lock_train.l_pid = getpid();
		int fd_train = open("database/train", O_RDWR);
		fcntl(fd_train, F_SETLKW, &lock_train);
		
		//read the corresponding train record.
		lseek(fd_train,(record_b.train_id)*sizeof(struct train),SEEK_SET);
		struct train record_t;
		read(fd_train,&record_t,sizeof(record_t));
		
		//update the train available seats and mark as deleted
		record_t.available += record_b.seats;
		record_b.seats = 0;
		record_b.deleted=1;

		lseek(fd_train,-1*sizeof(struct train),SEEK_CUR);	
		write(fd_train,&record_t,sizeof(record_t));
		lock_train.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock_train);
		close(fd_train);
		
		lseek(fd_book,-1*sizeof(struct booking),SEEK_CUR);
		write(fd_book,&record_b,sizeof(record_b));
		lock_booking.l_type = F_UNLCK;
		fcntl(fd_book, F_SETLK, &lock_booking);
		close(fd_book);
		
		valid = 1;
		//inform client if operation was successful
		write(client_sd,&valid,sizeof(valid));
		
		
	}
}

//sign up menu
void signup(int client_sd){
	
	//read the type of account,name,password from user.
	int type;
	char name[NAMELEN],pswd[PASSLEN];
	memset(pswd, 0, PASSLEN*sizeof(pswd[0]));
	read(client_sd, &type, sizeof(type));
	read(client_sd, &name, sizeof(name));
	read(client_sd, &pswd, sizeof(pswd));
	
	//lock the user database to add new user.
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();
	int user_fd = open("database/user",O_RDWR);
	fcntl(user_fd,F_SETLKW, &lock);
	
	//check if user database is empty
	int fp = lseek(user_fd, 0, SEEK_END);
	
	struct user u;
	u.deleted=0;
	
	//if database is empty start with user_id=1. and add new record.
	if(fp==0){
		u.user_id = 1;
		strcpy(u.name, name);
		strcpy(u.pswd, pswd);
		u.type=type;
		write(user_fd, &u, sizeof(u));
		//inform client about his login_id.
		write(client_sd, &u.user_id, sizeof(u.user_id));
	}
	//if already some records are present. add the new record
	else{	
		lseek(user_fd, -1 * sizeof(struct user), SEEK_END);
		read(user_fd, &u, sizeof(u));
		u.user_id++;
		strcpy(u.name, name);
		strcpy(u.pswd, pswd);
		u.type=type;
		write(user_fd, &u, sizeof(u));
		//inform client about his login_id
		write(client_sd, &u.user_id, sizeof(u.user_id));
	}
	//unlock user database
	lock.l_type = F_UNLCK;
	fcntl(user_fd, F_SETLK, &lock);

	close(user_fd);
	
}


//train operations for admin. Note: view is also used by customer and agent.
void train_ops(int client_sd){
	//read the option from client.
	int choice;
	read(client_sd,&choice,sizeof(choice));
	
	//add new train
	if(choice==1){
		
		//prepare new train record to add  	
		struct train record_t;		
		char tname[tname_LEN];
		int train_id = 0;
		read(client_sd,&tname,sizeof(tname));
		record_t.train_id = train_id;
		strcpy(record_t.train_name,tname);
		record_t.capacity = DEF_SEATS;	//default values can be changed in conf file			
		record_t.available = DEF_SEATS;
		record_t.deleted=0;
		
		
		//lock train database
		struct flock lock;
		lock.l_whence = SEEK_SET;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_pid = getpid();
		int fd_train = open("database/train", O_RDWR);
		fcntl(fd_train, F_SETLKW, &lock);
		
		
		//check if database already has any trains
		int fp = lseek(fd_train, 0, SEEK_END); 
		
		int valid=0;	
		
		//if database is empty
		if(fp == 0){
			int x=write(fd_train, &record_t, sizeof(record_t));
			if(x!=-1)
				valid=1;
		}
		else{	
			//go to last record in train database.
			lseek(fd_train, -1 * sizeof(struct train), SEEK_END);
			struct train last_rec;
			read(fd_train, &last_rec, sizeof(last_rec));
			
			//new id= last_rec_id+1 and write it in database
			record_t.train_id = last_rec.train_id + 1;
			int x=write(fd_train, &record_t, sizeof(record_t));
			if(x!=-1)
				valid=1;
		}
		
		//unlock train database
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
		
		//inform if operation was successful.
		write(client_sd,&valid,sizeof(valid));
		
		
	}
	
	//view trains
	else if(choice==2){					
	
		//lock train database as readlock
		struct flock lock;
		lock.l_whence = SEEK_SET;
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_pid = getpid();
		int fd_train = open("database/train", O_RDONLY);
		fcntl(fd_train, F_SETLKW, &lock);
		

		int fp = lseek(fd_train, 0, SEEK_END);
		int total_trains = fp / sizeof(struct train);
		write(client_sd, &total_trains, sizeof(int));

		//display each train record one by one
		struct train record_t;
		lseek(fd_train,0,SEEK_SET);
		while(fp != lseek(fd_train,0,SEEK_CUR)){
			read(fd_train,&record_t,sizeof(record_t));
			write(client_sd,&record_t.train_id,sizeof(int));
			write(client_sd,&record_t.train_name,sizeof(record_t.train_name));
			write(client_sd,&record_t.capacity,sizeof(int));
			write(client_sd,&record_t.available,sizeof(int));
			write(client_sd,&record_t.deleted,sizeof(int));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
	}

	//modify train
	else if(choice==3){
	
		//show available trains to admin
		train_ops(client_sd);
		
		//accept the train id to modify
		int train_id;
		read(client_sd,&train_id,sizeof(train_id));
		
		//lock corresponding train record
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = (train_id)*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_pid = getpid();
		int fd_train = open("database/train", O_RDWR);
		fcntl(fd_train, F_SETLKW, &lock);
		
		//read corresponding train record
		struct train record_t;
		lseek(fd_train, (train_id)*sizeof(struct train), SEEK_SET);
		read(fd_train, &record_t, sizeof(struct train));
		
		//read client choice for modification to train
		int choice;
		read(client_sd,&choice,sizeof(int));
		
		
		int valid=1;
		//modify name
		if(choice==1){							
			int x=write(client_sd,&record_t.train_name,sizeof(record_t.train_name));
			if(x==-1)valid=0;
			x=read(client_sd,&record_t.train_name,sizeof(record_t.train_name));
			if(x==-1)valid=0;
			
		}
		//modify seats
		else if(choice==2){						
			int change;
			int x=write(client_sd,&record_t.capacity,sizeof(record_t.capacity));
			
			if(x==-1)valid=0;
			x=read(client_sd,&change,sizeof(change));
			
			if(x==-1)valid=0;
			record_t.capacity+=change;
			record_t.available+=change;
			
		}
		
		//write the modified record and unlock it.
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		write(fd_train, &record_t, sizeof(struct train));
		
		//inform client if operation is successful
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
		write(client_sd,&valid,sizeof(valid));
			
	}


	//drop a train from database.
	else if(choice==4){
		//show all trains to client.
		train_ops(client_sd);
		
		//read the train id which client wants to delete.
		int train_id;
		read(client_sd,&train_id,sizeof(train_id));
		
		//lock the train record which is to be deleted.
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = (train_id)*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		lock.l_pid = getpid();
		int fd_train = open("database/train", O_RDWR);
		fcntl(fd_train, F_SETLKW, &lock);
		
		//read the required train record in record_t and modify its name as deleted.
		struct train record_t;
		record_t.deleted=1;
		lseek(fd_train, (train_id)*sizeof(struct train), SEEK_SET);
		read(fd_train, &record_t, sizeof(struct train));

		
		int valid=1;
		//update modified record
		lseek(fd_train, -1*sizeof(struct train), SEEK_CUR);
		int x=write(fd_train, &record_t, sizeof(struct train));
		if(x==-1)valid=0;
		
		//unlock train database.
		lock.l_type = F_UNLCK;
		fcntl(fd_train, F_SETLK, &lock);
		close(fd_train);
		
		//inform client if train was successfully dropped.
		write(client_sd,&valid,sizeof(valid));
			
	}	
}

void user_ops(int client_sd){
	int valid=0;
	//read the operation client wants to perform
	int choice;
	read(client_sd,&choice,sizeof(choice));
	
	//add new user
	if(choice==1){    
	
		//read new user credentials from user
		char name[NAMELEN],pswd[PASSLEN];
		memset(pswd, 0, PASSLEN*sizeof(pswd[0]));	
		int type;
		read(client_sd, &type, sizeof(type));
		read(client_sd, &name, sizeof(name));
		read(client_sd, &pswd, sizeof(pswd));
		
		//lock the user database
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		int user_fd = open("database/user", O_RDWR);
		fcntl(user_fd, F_SETLKW, &lock);



		
		struct user record_user;
		record_user.deleted=0;
		int fp = lseek(user_fd, 0, SEEK_END);
		
		//if database is empty start with id=1;
		if(fp==0){
			record_user.user_id = 1;
			strcpy(record_user.name, name);
			strcpy(record_user.pswd, pswd);
			record_user.type=type;
			write(user_fd, &record_user, sizeof(record_user));
		}
		//if database already has some
		else{
			fp = lseek(user_fd, -1 * sizeof(struct user), SEEK_END);
			read(user_fd, &record_user, sizeof(record_user));
			record_user.user_id++;
			strcpy(record_user.name, name);
			strcpy(record_user.pswd, pswd);
			record_user.type=type;
			write(user_fd, &record_user, sizeof(record_user));
		}
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
		valid = 1;
		//inform client if user is created and its id.
		write(client_sd,&valid,sizeof(int));
		write(client_sd, &record_user.user_id, sizeof(record_user.user_id));
		
	}

	//view all users.
	else if(choice==2){	 	
		
		//lock user database as read lock.
		struct flock lock;
		lock.l_type = F_RDLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		int user_fd = open("database/user", O_RDONLY);
		fcntl(user_fd, F_SETLKW, &lock);
		
		struct user record_user;
		
		int fp = lseek(user_fd, 0, SEEK_END);
		int total_users = fp / sizeof(struct user);
		//inform client about total users.
		write(client_sd, &total_users, sizeof(int));

		lseek(user_fd,0,SEEK_SET);
		while(fp != lseek(user_fd,0,SEEK_CUR)){
			read(user_fd,&record_user,sizeof(record_user));
			write(client_sd,&record_user.user_id,sizeof(int));
			write(client_sd,&record_user.name,sizeof(record_user.name));
			write(client_sd,&record_user.type,sizeof(int));
			write(client_sd,&record_user.deleted,sizeof(int));
		
		}
		valid = 1;
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
	}


	//modify user
	else if(choice==3){
		//show all users to client.
		user_ops(client_sd);
		
		//read the user id to modify
		int uid;
		read(client_sd,&uid,sizeof(uid));

		//lock particular user record.
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start =  (uid-1)*sizeof(struct user);
		lock.l_len = sizeof(struct user);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		int user_fd = open("database/user", O_RDWR);
		fcntl(user_fd, F_SETLKW, &lock);
		
		char pass[PASSLEN];
		struct user record_user;
		lseek(user_fd, (uid-1)*sizeof(struct user), SEEK_SET);
		read(user_fd, &record_user, sizeof(struct user));
		
		int choice;
		int valid=0;
		read(client_sd,&choice,sizeof(int));
		
		if(choice==1){					// update name
			write(client_sd,&record_user.name,sizeof(record_user.name));
			read(client_sd,&record_user.name,sizeof(record_user.name));
			valid=1;
			
		}
		else if(choice==2){			
			write(client_sd,&record_user.pswd,sizeof(record_user.pswd));
			read(client_sd,&record_user.pswd,sizeof(record_user.pswd));
			valid=1;
		}
	
		lseek(user_fd, -1*sizeof(struct user), SEEK_CUR);
		write(user_fd, &record_user, sizeof(struct user));
		
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
		write(client_sd,&valid,sizeof(valid));	
		
			
	}

	//delete a user
	else if(choice==4){		
	
		//display all users to client.			
		user_ops(client_sd);
		
		//read the id of user to delete
		int uid;
		read(client_sd,&uid,sizeof(uid));
		
		//lock particular user
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start =  (uid-1)*sizeof(struct user);
		lock.l_len = sizeof(struct user);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		int user_fd = open("database/user", O_RDWR);
		fcntl(user_fd, F_SETLKW, &lock);
		
		
		int valid=0;
		
		struct user record_user;
		lseek(user_fd, (uid-1)*sizeof(struct user), SEEK_SET);
		read(user_fd, &record_user, sizeof(struct user));
		record_user.deleted=1;
		lseek(user_fd, -1*sizeof(struct user), SEEK_CUR);
		write(user_fd, &record_user, sizeof(struct user));
		
		//unlock database
		lock.l_type = F_UNLCK;
		fcntl(user_fd, F_SETLK, &lock);
		close(user_fd);
		
		valid=1;
		write(client_sd,&valid,sizeof(valid));
			
	}
}


