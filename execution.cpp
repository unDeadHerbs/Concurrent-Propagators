#include "properator.hpp"

#include <algorithm>
#include <stdio.h>

// std::cout has memory initialization problems that annoy the LLVM
// sanitizers.  Making an unbuffered version that's safe.
struct Printer{} cout;
Printer& operator<<(Printer& o,int const rhs){
	printf("%d",rhs);
	return o;
}
Printer& operator<<(Printer& o,std::string const rhs){
	printf("%s",rhs.c_str());
	return o;
}
Printer& operator<<(Printer& o,char const *rhs){
	printf("%s",rhs);
	return o;
}

//#define PrintLogMessages 1
#ifdef PrintLogMessages
#define LOG(X) do{cout<<X<<"\n";}while(0)
#else
#define LOG(X) do{}while(0)
#endif
#define LOG_ERROR(X) do{cout<<X<<"\n";}while(0)

//#define DEBUG 1
#ifdef DEBUG
#define DB(X) do{cout<<X<<"\n";}while(0)
#else
#define DB(X) do{}while(0)
#endif

// Variant Overload Visitor Code (cppreference)
template<class>
inline constexpr bool always_false_v = false;
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Core Types
UID new_uid(){
	static UID u=0;
	return ++u;
}
Printer& operator<<(Printer& o,LinkSpec const&rhs){
	return o<<"link{"<<rhs.from<<":"<<rhs.from_port<<" -> "<<rhs.to<<":"<<rhs.to_port<<"}";
}
Printer& operator<<(Printer& o,Message const&rhs){
	std::visit(overloaded{
			[&](int v){o<<v;},
			[&](float v){o<<v;},
			[&](std::string v){o<<v;},
			[&](UID id){o<<"id:"<<id;},
			[&](LinkSpec l){o<<l;},
			[&](std::vector<Message> vm){
				o<<"{";
				for(auto const&m:vm)
					o<<m<<",";
				o<<"}";
			}
		},rhs.body);
	return o;
}

// Execution Environment
std::vector<std::shared_ptr<Properator>> properators;
std::vector<std::shared_ptr<Channel>> channels;
std::vector<LinkSpec> purge_channels(UID block){
	//DB("Start  Purge Channels");
	auto is_attached=[&](std::shared_ptr<Channel> c){
		return c->info.to == block or c->info.from==block;};
	// This is very improvable but good enough for a moment.
	//DB("- Find");
	auto it=std::find_if(channels.begin(),channels.end(),is_attached);
	//DB("- Grab Headers");
	std::vector<LinkSpec> ret;
	for(;it!=channels.end();it++)
		ret.push_back((*it)->info);
	//DB("- Erase Each");
	channels.erase(std::remove_if(channels.begin(),channels.end(),is_attached),channels.end());
	//DB("Finish Purge Channels");
	return ret;
}
std::queue<std::pair<UID,Message>> system_messages;
void inform_next_of_kin(UID kin,std::string reason,LinkSpec link){
	//DB("Start  Inform Next of Kin");
	system_messages.push({kin,Message({std::vector<Message>({Message({reason}),Message({link})})})});
	//DB("Finish Inform Next of Kin");
}

bool main_loop_step(){
	//DB("starting main_loop_step");
	auto hand_message=[&](Message m, UID src, uint src_port,UID dest,uint dst_port){
		if(dest==0) return;  // The system isn't listening.
		if(src!=0){
			DB("  - handing message "<<src<<":"<<src_port<<"->"<<dest<<":"<<dst_port);
			DB("    - "<<m);}
		for(auto &p:properators)
			if(p->id==dest){
				p->receive(m,dst_port,src,src_port,p);
				return; // Assuming UIDs are Unique
			}

		if(src==0) return;
		LOG_ERROR("[Undeliverable] "<<src<<":"<<src_port<<"->"<<dest<<":"<<dst_port<<" Message: "<<m);

		auto next_of_kin = purge_channels(src);
		for(auto const&to_inform:next_of_kin)
			if(dest==to_inform.to)
				inform_next_of_kin(to_inform.from,"Not Found",to_inform);
			else
				inform_next_of_kin(to_inform.to,"Not Found",to_inform);
	};
	
	if(system_messages.size()){
		//DB("- Found System Message");
		auto [to,m]=system_messages.front();
		//DB("  :"<<m);
		system_messages.pop();
		hand_message(m,0,0,to,0);
		return true;
	}

	// Print all of the channels and their size
	//#ifdef DEBUG
	//for(auto const&c:channels)
	//	DB(c->info<<".size() == "<<c->size());
	//#endif

	// Perform all work to channel 0 first
	auto it=std::find_if(channels.begin(),channels.end(),
											 [](std::shared_ptr<Channel> c){return
													 c->has_message() and
													 c->info.to_port==0;});
	if(it==channels.end())
		it=std::find_if(channels.begin(),channels.end(),
										[](std::shared_ptr<Channel> c){return c->has_message();});
	if(it==channels.end()) return false;
	std::rotate(channels.begin(),it+1,channels.end());

	Message m = channels.back()->read();
	LinkSpec l= channels.back()->info;
	hand_message(m,l.from,l.from_port,l.to,l.to_port);
	return true;
}

/*
void GC(){
	// Remove all Properators that don't have a channel to them.  This
	// will need some more thinking about.  There might be loops, so a
	// list of roots is needed.  A Properator could be marked as a root
	// or daemon.
}
*/

void crash_or_shutdown(bool crash,UID id,Message log_message){
	//DB("Start  crash_or_shutdown");
	std::string reason=crash?"Crashed":"Shutting Down";
	if(crash)
		LOG_ERROR("["<<reason<<"] id:"<<id<<" Message:"<<log_message);
	else
		LOG("["<<reason<<"] id:"<<id<<" Message:"<<log_message);

	//DB("- Call Erase");
	properators.erase(std::remove_if(properators.begin(),properators.end(),
																	 [&](std::shared_ptr<Properator> p){return p->id==id;}),
										properators.end());

	auto next_of_kin = purge_channels(id);
	for(auto const&to_inform:next_of_kin)
		if(id==to_inform.to)
			inform_next_of_kin(to_inform.from,reason,to_inform);
		else
			inform_next_of_kin(to_inform.to,reason,to_inform);
	//DB("Finish crash_or_shutdown");
}

bool post(UID from,uint from_port,Message message){
	bool found=false;
	for(auto&c:channels)
		if(c->info.from==from and c->info.from_port==from_port){
			found=true;
			c->send(message);
		}
	if(!found)LOG("[Missing Channel] "<<from<<":"<<from_port<<" -> *");
	return found;
}
bool post(UID from,uint from_port,UID to,Message message){
	bool found=false;
	for(auto&c:channels)
		if(c->info.from==from and c->info.from_port==from_port and c->info.to==to){
			found=true;
			c->send(message);
		}
	if(!found)LOG_ERROR("[Missing Channel] "<<from<<":"<<from_port<<" -> "<<to<<":*");
	return found;
}
bool post(UID from,uint from_port,UID to, uint to_port,Message message){
	bool found=false;
	for(auto&c:channels)
		if(c->info.from==from and c->info.from_port==from_port and
			 c->info.to==to and c->info.to_port==to_port){
			found=true;
			c->send(message);
		}
	if(!found and from==0){
		auto c=make_channel<BasicChannel>({from,from_port,to,to_port});
		c->send(message);
		return true;
	}
	if(!found)LOG_ERROR("[Missing Channel] "<<LinkSpec({from,from_port,to,to_port}));
	return found;
}

// Builtin Types Implementation
void BasicChannel::send(Message m){v.push(m);}
Message BasicChannel::read(){
	auto m = v.front();
	v.pop();
	return m;
}
bool BasicChannel::has_message() const{return v.size();}

void OnlyLatests::send(Message m){v=m;}
Message OnlyLatests::read(){
	auto m = *v;
	v={};
	return m;
}
bool OnlyLatests::has_message() const{return bool(v);}

void Relay::receive(Message m, uint port,UID,uint,std::shared_ptr<Properator>){
	switch(port){
	case 0:
		if(std::holds_alternative<std::vector<Message>>(m.body)){
			auto v = std::get<std::vector<Message>>(m.body);
			if(v.size())
				if(std::holds_alternative<std::string>(v[0].body)){
					auto vv = std::get<std::string>(v[0].body);
					if(vv=="Shutting Down")
						return;
				}
		}
		crash_or_shutdown(true,id,m);
		break;
	default:
		post(id,port,m);
	}
}

void MessageLogger::receive(Message m, uint port,UID from,uint from_port,std::shared_ptr<Properator>){
	switch(port){
	case 0:
		if(std::holds_alternative<std::vector<Message>>(m.body)){
			auto v = std::get<std::vector<Message>>(m.body);
			if(v.size())
				if(std::holds_alternative<std::string>(v[0].body)){
					auto vv = std::get<std::string>(v[0].body);
					if(vv=="Shutting Down")
						return;
				}
		}
		crash_or_shutdown(true,id,m);
		break;
	case 1:
		cout<<"["<<id<<":"<<port<<"] Received Message from ["<<from<<":"<<from_port<<"] : "<<m<<"\n";
		break;
	default:
		crash_or_shutdown(true,id,m);
	}
}

