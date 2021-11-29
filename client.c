#include "common.h"
#include "conf.h"

void service_req(int sd);
void post_login_ops(int sd,int type);
void booking_operation(int sd,int choiusece);
void train_operations(int sd,int choice);
void user_operations(int sd,int choice);
void login(int sd);
void signup(int sd);

int main(void) { 
	//connect to server and service the request.
    	struct sockaddr_in server; 
	char *ip = IP; 
     
    	int sd = socket(AF_INET, SOCK_STREAM, 0); 
    	if (sd == -1) { 
       	printf("Could not create socket"); 
    	} 
  
    
    	server.sin_addr.s_addr = inet_addr(ip); 
    	server.sin_family = AF_INET; 
    	server.sin_port = htons(PORT); 
   	
    	if (connect(sd, (struct sockaddr*)&server, sizeof(server)) == -1)
       	perror("Unable to connect to server"); 
    	
	service_req(sd);
	
    	close(sd); 
    	
	return 0; 
} 

// After connection Establishment.This is the first function that is executed

void login(int sd){
		int id;
		char pswd[PASSLEN];
		printf("\nEnter Login ID:");
		scanf("%d", &id);
		strcpy(pswd,getpass("\nEnter Password:"));
		int x=write(sd, &id, sizeof(id));
		if(x==-1)printf("error writing to server. make sure you entered your login id which is an integer.\n");
		write(sd, &pswd, sizeof(pswd));
		
		int type,valid;
		read(sd, &valid, sizeof(valid));
		if(valid){
			printf("Authentication Successful. Welcome!\n");
			read(sd,&type,sizeof(type));
			post_login_ops(sd,type);
			//system("clear");
		}
		else{
			printf("Authentication failed. Try again\n");
		}
		
		service_req(sd);
}

void signup(int sd){
		int type,id;
		char name[NAMELEN],pswd[PASSLEN],secret_pin[6];
		printf("\nEnter The Type Of Account:: \n");
		printf("1. Admin\n");
		printf("2. Agent\n");
		printf("3. Customer\n");
		printf("Enter choice: ");
		scanf("%d", &type);
		if(type<1 || type>3){
			printf("Invalid choice. Enter 1,2 or 3\n");
			signup(sd);
		}
		
		else{
			printf("Enter Your Name: \n");
			scanf("%s", name);
			strcpy(pswd,getpass("Enter password: \n"));


			write(sd, &type, sizeof(type));
			write(sd, &name, sizeof(name));
			write(sd, &pswd, strlen(pswd));
		
			read(sd, &id, sizeof(id));
			printf("\nYou ID for login is  ***%d***. Please remember it for login next time.\n", id);
			service_req(sd);
		}
}

void service_req(int sd){
	int choice;
	int valid;
	printf("\n\n\t\t ***Weclome to Online Ticket Booking System***\t\t\t\t\n");
	printf("\nChoose among the following\n");
	printf("1. Log In\n");
	printf("2. Sign Up New Account\n");
	printf("3. Exit:\n");
	scanf("%d", &choice);
	write(sd, &choice, sizeof(choice));
	
	//service loging req
	if (choice == 1){	
		//verify credentials by sending to server	
		login(sd);
	}
	
	//service sign up req
	else if(choice == 2){					
		signup(sd);
	}
	//logout
	else							
		return;
	
}

//After login operations.
void post_login_ops(int sd,int type){
	int choice;
	
	//Customer or Agent operations
	if(type==2 || type==3){
		printf("\n1. Book Ticket\n");
		printf("2. View Bookings\n");
		printf("3. Update Booking\n");
		printf("4. Cancel booking\n");
		printf("5. Logout\n");
		scanf("%d",&choice);
		write(sd,&choice,sizeof(choice));
		booking_operation(sd,choice);
		
		//display same menu again if not chosen to log out.
		if(choice!=5)
			post_login_ops(sd,type);
	}
	
	//Admin operations
	else if(type==1){	
		printf("\nChoose among the following options:\n");				
		printf("1. CRUD operations on train\n");
		printf("2. CRUD operations on user\n");
		printf("3. Logout\n");
		
		scanf("%d",&choice);
		write(sd,&choice,sizeof(choice));
			//train_ops
			if(choice==1){
				printf("\n1. Add train\n");
				printf("2. View train\n");
				printf("3. Modify train\n");
				printf("4. Delete train\n");
				printf("Please make a choice: \n");
				scanf("%d",&choice);	
				write(sd,&choice,sizeof(choice));
				train_operations(sd,choice);
				post_login_ops(sd,type);
			}
			//user_ops
			else if(choice==2){
				printf("\n1. Add User\n");
				printf("2. View all users\n");
				printf("3. Modify user\n");
				printf("4. Delete user\n");
				printf("Please make a choice: \n");
				scanf("%d",&choice);
				write(sd,&choice,sizeof(choice));
				user_operations(sd,choice);
				post_login_ops(sd,type);
			
			}
			else if(choice==3)
				return;
	}	
	
}


//booking operations by customer and agent.
void booking_operation(int sd,int choice){
	int valid =0;
	//booking ticket
	if(choice==1){										
		int view=2,train_id,seats;
		write(sd,&view,sizeof(int));
		train_operations(sd,view);
		printf("\nEnter train id for boooking: ");
		scanf("%d",&train_id);
		write(sd,&train_id,sizeof(train_id));
				
		printf("\nEnter number of required seats: ");
		scanf("%d",&seats);
		write(sd,&seats,sizeof(seats));
	
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("Ticket booking successfully.\n");
		else
			printf("Ticket booking failed. Not enough seats available.\n");

	}
	
	//viewing booking
	else if(choice==2){									
		int total_bookings;
		int id,train_id,seats;
		read(sd,&total_bookings,sizeof(total_bookings));

		printf("\tB_id\tT_id\total_seats\n");
		while(total_bookings--){
			read(sd,&id,sizeof(id));
			read(sd,&train_id,sizeof(train_id));
			read(sd,&seats,sizeof(seats));
			
			//print only valid bookings. dont show cancelled bookings
			if(seats!=0)
				printf("\t%d\t%d\t%d\n",id,train_id,seats);
		}

	}
	
	//update booking
	else if(choice==3){									
		int bid,valid;
		booking_operation(sd,2);
		printf("\nEnter the booking id you want to modify: ");
		scanf("%d",&bid);
		write(sd,&bid,sizeof(bid));

		int change;
		printf("\nEnter the change in seats you want to make.Enter the exact seats. E.g. \n Enter 2 if you want to increase 2 seats.\n Enter -2 if you want to decrease 2 seats: ");
		scanf("%d",&change);
		write(sd,&change,sizeof(change));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nBooking updated successfully.\n");
		else
			printf("\nUpdation failed. No more seats available. Or invalid change\n");
	
	}
	
	//cancel a booking
	else if(choice==4){									
		int bid,valid;
		booking_operation(sd,2);
		printf("\nEnter the booking id that you want to cancel: \n");
		scanf("%d",&bid);
		write(sd,&bid,sizeof(bid));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nCancellation Successful\n");
		else
			printf("\nCancellation failed.\n");
		
	}
	
	
}

//Admin operations on train
void train_operations(int sd,int choice){
	int valid = 0;
	//add new train
	if(choice==1){			
		char tname[tname_LEN];
		printf("\nEnter the name of new train\n");
		scanf("%s",tname);
		write(sd, &tname, sizeof(tname));
		read(sd,&valid,sizeof(valid));	
		if(valid)
			printf("\nTrain added successfully\n");
		else{
			printf("\nError adding new train\n");
		}

	}
	//view train
	else if(choice==2){			
		int total_trains;
		int train_id;
		char tname[tname_LEN];
		int total_seats;
		int avail_seats;
		int deleted;
		read(sd,&total_trains,sizeof(total_trains));

		printf("\tid\tName\tMax\tAvail\n");
		while(total_trains--){
			read(sd,&train_id,sizeof(train_id));
			read(sd,&tname,sizeof(tname));
			read(sd,&total_seats,sizeof(total_seats));
			read(sd,&avail_seats,sizeof(avail_seats));
			read(sd,&deleted,sizeof(deleted));
			if(!deleted)
				printf("\t%d\t%s\t%d\t%d\n",train_id,tname,total_seats,avail_seats);
		}
	
	}
	
	//update train info
	else if (choice==3){			
		int total_seats,choice=2,valid=0,train_id;
		char tname[tname_LEN];
		write(sd,&choice,sizeof(int));
		train_operations(sd,choice);
		printf("\nEnter the train number you want to modify: ");
		scanf("%d",&train_id);
		write(sd,&train_id,sizeof(train_id));
		
		printf("\n1. Train Name\n2. Total Seats\n");
		printf("Your Choice: ");
		scanf("%d",&choice);
		write(sd,&choice,sizeof(choice));
		
		if(choice==1){
			read(sd,&tname,sizeof(tname));
			printf("\nCurrent name: %s",tname);
			printf("\nUpdated name:");
			scanf("%s",tname);
			write(sd,&tname,sizeof(tname));
		}
		else if(choice==2){
			read(sd,&total_seats,sizeof(total_seats));
			printf("\nCurrent max capacity: %d",total_seats);
			printf("\nEnter the exact change you want in seats. -ve for decreaseing and +ve for increaseing: ");
			int change;
			scanf("%d",&change);
			write(sd,&change,sizeof(change));
		}
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nTrain data updated successfully\n");
		else printf("\nError modifying train\n");
	
	}
	
	//delete a train
	else if(choice==4){				
		int choice=2,train_id,valid=0;
		write(sd,&choice,sizeof(int));
		train_operations(sd,choice);
		
		printf("\nEnter the train ID you want to delete: ");
		scanf("%d",&train_id);
		write(sd,&train_id,sizeof(train_id));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nTrain deleted successfully\n");
		
	}
	
}

//User operations
void user_operations(int sd,int choice){
	int valid = 0;
	//add new user
	if(choice==1){
		int type,id;
		char name[NAMELEN],pswd[PASSLEN];
		printf("\nEnter The Type Of Account: ");
		printf("\n2. Agent\n3. Customer\n");
		printf("Enter choice: ");
		scanf("%d", &type);
		printf("Enter Your Name: \n");
		scanf("%s", name);
		strcpy(pswd,getpass("Enter password: \n"));


		write(sd, &type, sizeof(type));
		write(sd, &name, sizeof(name));
		write(sd, &pswd, strlen(pswd));
		
		
		read(sd,&valid,sizeof(valid));	
		if(valid){
			read(sd,&id,sizeof(id));
			printf("\nYour login id is ***%d***. Remember for next login\n", id);
		}
	
	}
	
	//view
	else if(choice==2){						
		int total_users;
		int id,type;
		int deleted;
		char user_name[NAMELEN];
		read(sd,&total_users,sizeof(total_users));

		printf("\tid\tname\ttype\n");
		while(total_users--){
			read(sd,&id,sizeof(id));
			read(sd,&user_name,sizeof(user_name));
			read(sd,&type,sizeof(type));
			read(sd,&deleted,sizeof(deleted));
			if(!deleted)
				printf("\t%d\t%s\t%d\n",id,user_name,type);
		}

	}

	//update existing user
	else if (choice==3){	
		int choice=2,valid=0,uid;
		char name[NAMELEN],pass[PASSLEN];
		write(sd,&choice,sizeof(int));
		user_operations(sd,choice);
		printf("\nEnter the user_id you want to modify: ");
		scanf("%d",&uid);
		write(sd,&uid,sizeof(uid));
		
		printf("\n1. User Name\n2. pswd\n");
		printf("Enter Choice: ");
		scanf("%d",&choice);
		write(sd,&choice,sizeof(choice));
		
		if(choice==1){
			read(sd,&name,sizeof(name));
			printf("\nCurrent name: %s",name);
			printf("\nEnter new username:");
			scanf("%s",name);
			write(sd,&name,sizeof(name));
			read(sd,&valid,sizeof(valid));
		}
		else if(choice==2){
			read(sd,&pass,sizeof(pass));
			printf("Current password: %s",pass);
			printf("\nEnter new password:");
			scanf("%s",pass);
			write(sd,&pass,sizeof(pass));
			read(sd,&valid,sizeof(valid));
		}
		if(valid)printf("\nOperation successful\n");
		else printf("\nOperation Unsuccessful\n");
		
			
	}
		
	
	
	//delete a user
	else if(choice==4){			
		int choice=2,uid,valid=0;
		write(sd,&choice,sizeof(int));
		user_operations(sd,choice);
		
		printf("\nEnter the id you want to delete: ");
		scanf("%d",&uid);
		write(sd,&uid,sizeof(uid));
		read(sd,&valid,sizeof(valid));
		if(valid)
			printf("\nUser deleted successfully\n");
		else printf("\nError deleting user\n");
	
	}
}


