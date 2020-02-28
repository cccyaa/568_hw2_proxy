#include <iostream>
#include <fstream>

using namespace std;


class logger{
	string uniqueID;
	string requestLine;

public:
	logger(int ui){
		uniqueID=to_string(ui);
	}

	void write(string log){
		ofstream myfile;
		myfile.open("log.txt",ios::app);
		myfile<<log;
		myfile.close();
	}
	string getCurrentTime(){
		time_t rawtime;
		struct tm* timeinfo;
		time ( &rawtime );
  		timeinfo = localtime ( &rawtime );
  		return asctime(timeinfo);
	}

	void receiveNewRequest(string clientAddr,string requestline){//104: "GET www.bbc.co.uk/ HTTP/1.1" from 1.2.3.4 @ Sun Jan 1 22:58:17 2017 
		requestLine=requestline;
		string log=uniqueID+": "+requestLine+" from "+clientAddr+" @ "+getCurrentTime();
		write(log);
	}
	void requestRequest(string serverAddr){//ID: Requesting "REQUEST" from SERVER 
		string log=uniqueID+": Requesting \""+requestLine+"\" from "+serverAddr+"\n";
		write(log);
	}

	void receiveResponse(string serverAddr,string responseLine){//ID:  Received "RESPONSE" from SERVER 
		string log=uniqueID+": Received \""+responseLine+"\" from"+serverAddr+"\n";
		write(log);
	}

	void respondResponse(string responseLine){//ID: Responding "RESPONSE" 
		string log=uniqueID+": Responding \""+responseLine+"\""+"\n";
		write(log);
	}

	void notInCache(){
		string log=uniqueID+": not in cache\n";
		write(log);
	}

	void expireCache(string EXPIRETIME){
		string log=uniqueID+": in cache, but expired at "+EXPIRETIME+"\n";
		write(log);
	}

	void revalidationCache(){
		string log=uniqueID+": in cache, but requires validation\n";
		write(log);
	}

	void validCache(){
		string log=uniqueID+": in cache, valid\n";
		write(log);
	}

	void errorMessage(string message){
		string log=uniqueID+": ERROR "+message+"\n";
		write(log);
	}

	void noteMessage(string message){
		string log=uniqueID+": NOTE "+message+"\n";
		write(log);
	}

	void warningMessage(string message){
		string log=uniqueID+": WARNING "+message+"\n";
		write(log);
	}

	void tunnelClose(){
		string log=uniqueID+": Tunnel closed"+"\n";
		write(log);
	}
};