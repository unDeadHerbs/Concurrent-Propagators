#ifndef __PROPERATOR__
#define __PROPERATOR__

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

// Core Types
typedef long int UID;
UID new_uid();
struct LinkSpec{
	UID from;
	unsigned int from_port;
	UID to;
	unsigned int to_port;
};
struct Message{
	// TODO: make this a std::any
	std::variant<int,float,std::string,UID,LinkSpec,std::vector<Message>> body;
};
struct Channel{
	LinkSpec info;
	Channel(LinkSpec _info):info(_info){}
	virtual void send(Message)=0;
	virtual Message read()=0;
	virtual bool has_message() const=0;
	virtual ~Channel()=default;
};
struct Properator{ // propagator or operator
	UID id;
	Properator(UID _id):id(_id){}
	// Port 0 is for construction and system messages
	// The self pointer is so that the cleanup happens after the function finishes.
	virtual void receive(Message, uint port,UID from,uint from_port,std::shared_ptr<Properator> self)=0;
	virtual ~Properator()=default;
};

extern std::vector<std::shared_ptr<Properator>> properators;
extern std::vector<std::shared_ptr<Channel>> channels;

bool post(UID from,uint from_port,Message message);
bool post(UID from,uint from_port,UID to,Message message);
bool post(UID from,uint from_port,UID to, uint to_port,Message message);

struct BasicChannel:Channel{
	std::queue<Message> v;
	BasicChannel(LinkSpec link):Channel(link){};
	void send(Message m)override;
	Message read()override;
	bool has_message() const override;
};
struct OnlyLatests:Channel{
	std::optional<Message> v;
	OnlyLatests(LinkSpec link):Channel(link){};
	void send(Message m)override;
	Message read()override;
	bool has_message() const override;
};
struct Relay:Properator{
	std::string value;
	Relay(UID id):Properator(id){}
	void receive(Message m, uint port,UID,uint,std::shared_ptr<Properator>)override;
};
struct MessageLogger:Properator{
	MessageLogger(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint from_port,std::shared_ptr<Properator>);
};

bool main_loop_step();// Return if did anything.  Only for example version.
void crash_or_shutdown(bool crash,UID id,Message log_message);

template<typename T> UID spawn_properator(){
	// TODO: Constrain T to be a Properator.
	UID id=new_uid();
	auto p=std::make_shared<T>(id);
	properators.push_back(p);
	return id;
}
template<typename T> std::shared_ptr<Channel> make_channel(LinkSpec linkspec){
	// TODO: Constrain T to be a Channel.
	auto c=std::make_shared<T>(linkspec);
	channels.push_back(c);
	return c;
}
#endif
