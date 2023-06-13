#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <stdio.h>
#include <string>
#include <variant>

// for operator ""s
using namespace std::string_literals;

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
typedef long int UID;
UID new_uid(){
	static UID u=0;
	return ++u;
}
struct LinkSpec{
	UID from;
	uint from_port;
	UID to;
	uint to_port;
};
Printer& operator<<(Printer& o,LinkSpec const&rhs){
	return o<<"link{"<<rhs.from<<":"<<rhs.from_port<<" -> "<<rhs.to<<":"<<rhs.to_port<<"}";
}
struct Message{
	// can be any of
	// - basic
	//   - int
	//   - float
	//   - string
	//   - UID
	//   - LinkSpec
	// - tuple/vector of messages
	std::variant<int,float,std::string,UID,LinkSpec,std::vector<Message>> body;
	// can be serialized for log messages
};
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
struct Channel{
	LinkSpec info;
	//Channel(LinkSpec _info):info(_info){}
	Channel()=default;
	virtual void send(Message)=0;
	virtual Message read()=0;
	virtual bool has_message() const=0;
	#ifdef DEBUG
	virtual size_t size() const=0;
	#endif
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

// Execution Environment
std::vector<std::shared_ptr<Properator>> properators;
template<typename T> UID spawn_properator(){
	// TODO: Constrain T to be a Properator.
	UID id=new_uid();
	LOG("[Spawning "<<typeid(T).name()<<"] id:"<<id);
	auto p=std::make_shared<T>(id);
	//p->id=id;
	properators.push_back(p);
	return id;
}
std::vector<std::shared_ptr<Channel>> channels;
template<typename T> std::shared_ptr<Channel> make_channel(LinkSpec linkspec){
	// TODO: Constrain T to be a Channel.
	LOG("[New Channel] "<<linkspec);
	auto c=std::shared_ptr<T>(new T);//should be std::make_shared<T>() but bug in libc++ (probably)
	c->info=linkspec;
	channels.push_back(c);
	return c;
}
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

bool main_loop_step(){// Return if did anything.  Only for example version.
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

struct BasicChannel;

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

// broadcast, post to all wires from that port.
// return false if no channels exist
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

// Builtin Types
struct BasicChannel:Channel{
	std::queue<Message> v;
	BasicChannel()=default;
	void send(Message m){
		v.push(m);
	}
	Message read(){
		auto m = v.front();
		v.pop();
		return m;
	}
	bool has_message() const{return v.size();}
	#ifdef DEBUG
	size_t size() const{return v.size();}
	#endif
};
struct OnlyLatests:Channel{
	std::optional<Message> v;
	OnlyLatests()=default;
	void send(Message m){
		v=m;
	}
	Message read(){
		auto m = *v;
		v={};
		return m;
	}
	bool has_message() const{return bool(v);}
	#ifdef DEBUG
	size_t size() const{return bool(v);}
	#endif
};
struct Relay:Properator{
	Relay(UID id):Properator(id){}
	std::string value;
	void receive(Message m, uint port,UID,uint,std::shared_ptr<Properator>){
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
	~Relay()=default;
};

// Hello World Example
struct StringHolder:Properator{
	StringHolder(UID id):Properator(id){}
	std::string value;
	void receive(Message m, uint port,UID,uint,std::shared_ptr<Properator>){
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
			if(std::holds_alternative<std::string>(m.body)){
				auto v = std::get<std::string>(m.body);
				if(v==value) return; // Deduplicate
				post(id,1,m);
				return;
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
	~StringHolder()=default;
};
struct MessageLogger:Properator{
	MessageLogger(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint from_port,std::shared_ptr<Properator>){
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
	~MessageLogger()=default;
};
void hello_example(){
	DB("staring Hello");
	auto s_hold = spawn_properator<StringHolder>();
	auto printer = spawn_properator<MessageLogger>();
	make_channel<BasicChannel>({s_hold,1,printer,1});
	post(0,0,s_hold,1,Message({"Hello, World!"}));
	while(main_loop_step());
	crash_or_shutdown(false,printer,Message({"Example Over"}));
	while(main_loop_step());
	crash_or_shutdown(false,s_hold,Message({"Example Over"}));
	while(main_loop_step());
	DB("ending Hello");
}

// Factorial Example
struct FactorialCalculator:Properator{
	FactorialCalculator(UID id):Properator(id){
		// This isn't recommended to be part of a constructor, just do the
		// work locally; but, this is a test case.
		make_channel<BasicChannel>({id,2,id,2});
	}
	void receive(Message m, uint port,UID,uint,std::shared_ptr<Properator>){
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
			// factorial:1 n -> (n,n-1,return id, return port -> factorial:2)
			if(std::holds_alternative<int>(m.body)){
				auto n = std::get<int>(m.body);
				post(id,2,id,2,Message({std::vector<Message>({
							Message({1}),
							Message({n})})}));
				return;
			}
			crash_or_shutdown(true,id,m);
			break;
		case 2:
			// factorial:2 a,n -> (a*n,n-1 -> factorial:2)
			// factorial:2 a,0 -> (a -> listeners on port 1)
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2){
					if(std::holds_alternative<int>(v[0].body) and
						 std::holds_alternative<int>(v[1].body)){
						auto a=std::get<int>(v[0].body);
						auto n=std::get<int>(v[1].body);
						if(n<0)
							crash_or_shutdown(true,id,m);
						else if(n==0)
							post(id,1,Message({a}));
						else
							post(id,2,id,2,Message({std::vector<Message>({
										Message({a*n}),
										Message({n-1})})}));
						break;
					}
				}
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
	~FactorialCalculator()=default;
};
void factorial_example(){
	DB("staring Fact");
	auto fact = spawn_properator<FactorialCalculator>();
	auto printer = spawn_properator<MessageLogger>();
	make_channel<BasicChannel>({fact,1,printer,1});
	post(0,0,fact,1,Message({5}));
	while(main_loop_step());
	crash_or_shutdown(false,printer,Message({"Example Over"}));
	while(main_loop_step());
	crash_or_shutdown(false,fact,Message({"Example Over"}));
	while(main_loop_step());
	DB("ending Fact");
}

// Sudoku Example
struct SudokuCell:Properator{
	bool can_be[9]={1,1,1, 1,1,1, 1,1,1};
	SudokuCell(UID id):Properator(id){}
	void receive(Message m, uint port,UID caller,uint,std::shared_ptr<Properator>){
		switch(port){
		case 0:
			// ["Shutting Down" . TAIL]
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size())
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Shutting Down")
							break;}}
			crash_or_shutdown(true,id,m);
			break;
		case 1:
			// [command,value] -> Do command
			// - ["Set",value] -> Set the value
			// - ["Ban",value] -> Ban the value
			// "Query"       -> reply
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2)
					if(std::holds_alternative<std::string>(v[0].body) and
						 std::holds_alternative<int>(v[1].body)){
						auto command=std::get<std::string>(v[0].body);
						auto val=std::get<int>(v[1].body);
						if(val<1 or val>9)
							crash_or_shutdown(true,id,m);
						bool changed=false;
						if(command=="Set"){
							for(int vl=0;vl<9;vl++)
								if(can_be[vl]!=(vl+1==val)){
									// If this updates to a positive, that should be a crash.
									can_be[vl]=(vl+1==val);
									changed=true;
								}
						}if(command=="Ban")
							if((changed=can_be[val-1]))
								can_be[val-1]=false;
						if(changed)
							if(can_be[0]+
								 can_be[1]+
								 can_be[2]+
								 can_be[3]+
								 can_be[4]+
								 can_be[5]+
								 can_be[6]+
								 can_be[7]+
								 can_be[8]
								 ==0)
								crash_or_shutdown(true,id,Message({std::vector<Message>({
													m,
													Message({int(can_be[0])}),
													Message({int(can_be[1])}),
													Message({int(can_be[2])}),
													
													Message({int(can_be[3])}),
													Message({int(can_be[4])}),
													Message({int(can_be[5])}),
													
													Message({int(can_be[6])}),
													Message({int(can_be[7])}),
													Message({int(can_be[8])})})}));
							else
								post(id,1,Message({std::vector<Message>({
													Message({int(can_be[0])}),
													Message({int(can_be[1])}),
													Message({int(can_be[2])}),
													
													Message({int(can_be[3])}),
													Message({int(can_be[4])}),
													Message({int(can_be[5])}),
													
													Message({int(can_be[6])}),
													Message({int(can_be[7])}),
													Message({int(can_be[8])})})}));
						break;
					}
			}else if(std::holds_alternative<std::string>(m.body)){
				auto command=std::get<std::string>(m.body);
				if(command=="Query"){
							post(id,port,caller,
									 Message({std::vector<Message>({
												Message({int(can_be[0])}),
												Message({int(can_be[1])}),
												Message({int(can_be[2])}),
															
												Message({int(can_be[3])}),
												Message({int(can_be[4])}),
												Message({int(can_be[5])}),
												
												Message({int(can_be[6])}),
												Message({int(can_be[7])}),
												Message({int(can_be[8])})})}));
							break;}}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
};
struct SudokuValueAtMostOnce:Properator{
	std::vector<UID> cells;
	// The states are only needed for more complex things, like pairs
	//std::map<UID,bool[9]> states;
	SudokuValueAtMostOnce(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint,std::shared_ptr<Properator>){
		switch(port){
		case 0:
			// ["Add Cell", UID]
			// ["Shutting Down" . TAIL]
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2)
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Add Cell")
							if(std::holds_alternative<UID>(v[1].body)){
								auto c=std::get<UID>(v[1].body);
								cells.push_back(c);
								if(cells.size()>9)
									crash_or_shutdown(true,id,Message({std::vector<Message>({
														Message({"Too many sub-cells: "s+std::to_string(cells.size())}),
														m})}));
								//for(int i=0;i<9;i++)
								//	states[c][i]=true;
								make_channel<BasicChannel>({id,1,c,1});
								make_channel<OnlyLatests>({c,1,id,1});
								post(id,1,c,1,Message({"Query"}));
								break;
							}
					}
				if(v.size())
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Shutting Down")
							break;}}
			crash_or_shutdown(true,id,m);
			break;
		case 1:
			// message::[current_state]
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==9){
					int found=0;
					for(int val=0;val<9;val++)
						if(!std::holds_alternative<int>(v[val].body))
							crash_or_shutdown(true,id,m);
						else
							if(std::get<int>(v[val].body))
								if(!found)
									found=val+1;
								else{
									found=0;
									break;}
					if(found)
						for(auto cell:cells)
							if(cell!=from)
								post(id,1,cell,1,Message({std::vector<Message>({
													Message({"Ban"}),
													Message({found})})}));
					break;
				}
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
};
struct SudokuValueAtLeastOnce:Properator{
	std::vector<UID> cells;
	std::map<UID,bool[9]> states;
	SudokuValueAtLeastOnce(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint,std::shared_ptr<Properator>){
		switch(port){
		case 0:
			// ["Add Cell", UID]
			// ["Shutting Down" . TAIL]
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2)
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Add Cell")
							if(std::holds_alternative<UID>(v[1].body)){
								auto c=std::get<UID>(v[1].body);
								cells.push_back(c);
								if(cells.size()>9)
									crash_or_shutdown(true,id,Message({std::vector<Message>({
														Message({"Too many sub-cells: "s+std::to_string(cells.size())}),
														m})}));
								for(int i=0;i<9;i++)
									states[c][i]=true;
								make_channel<BasicChannel>({id,1,c,1});
								make_channel<OnlyLatests>({c,1,id,1});
								post(id,1,c,1,Message({"Query"}));
								break;
							}
					}
				if(v.size())
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Shutting Down")
							break;}}
			crash_or_shutdown(true,id,m);
			break;
		case 1:
			// message::[current_state]
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==9){
					for(int val=0;val<9;val++)
						if(!std::holds_alternative<int>(v[val].body))
							crash_or_shutdown(true,id,m);
						else
							if(states[from][val]!=std::get<int>(v[val].body)){
								states[from][val]=std::get<int>(v[val].body);
								int found=0;
								for(auto cell:cells)
									if(states[cell][val])
										found++;
								if(found==1)
									for(auto cell:cells)
										if(states[cell][val])
											post(id,1,cell,1,Message({std::vector<Message>({
																Message({"Set"}),
																Message({val+1})})}));
							}
					break;
				}
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
};
struct SudokuGridDisplay:Properator{
	std::vector<UID> cells;
	std::map<UID,int> values;
	SudokuGridDisplay(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint,std::shared_ptr<Properator>){
		switch(port){
			case 0:
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2)
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Add Cell")
							if(std::holds_alternative<UID>(v[1].body)){
								auto c=std::get<UID>(v[1].body);
								cells.push_back(c);
								if(cells.size()>81)
									crash_or_shutdown(true,id,m); // TODO: Elaborate
								values[c]=0;
								make_channel<OnlyLatests>({id,1,c,1});
								make_channel<OnlyLatests>({c,1,id,1});
								post(id,1,c,1,Message({"Query"}));
								break;
							}
					}
				if(v.size())
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Shutting Down")
							break;}}
			// if cell crash or shutdown, crash
			// might change later with an actual grid rather than just a list of cells
			crash_or_shutdown(true,id,m);
			break;
		case 1:
			if(std::holds_alternative<std::string>(m.body)){
				auto v = std::get<std::string>(m.body);
				if(v=="Display"){
					if(cells.size()!=81){
						crash_or_shutdown(true,id,m); // TODO: Elaborate
						break;}
					cout<<"\n";
					for(int row=0;row<9;row++){
						if(row==3 or row==6)
							cout<<"---+---+---\n";
						for(int col=0;col<9;col++){
							auto val=values[cells[row*9+col]];
							if(col==3 or col==6)
								cout <<"|";
							if(val)
								cout<<val;
							else
								cout<<"_";}
						cout<<"\n";}
					break;
				}
			}else if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==9){
					int found=0;
					for(int val=0;val<9;val++)
						if(!std::holds_alternative<int>(v[val].body))
							crash_or_shutdown(true,id,m);
						else
							if(std::get<int>(v[val].body))
								if(!found)
									found=val+1;
								else{
									found=0;
									break;}
					values[from]=found;
					// Assert that that's an increase
					// assert that from was already in the map
					break;
				}
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
};
#ifdef DEBUG
struct SudokuGridVerboseDisplay:Properator{
	std::vector<UID> cells;
	std::map<UID,bool[9]> values;
	SudokuGridVerboseDisplay(UID id):Properator(id){}
	void receive(Message m, uint port,UID from,uint,std::shared_ptr<Properator>){
		auto display=[&](){
			cout<<"\n";
			for(size_t row=0;row<9;row++){
				if(row==3 or row==6)
					cout<<"---+---+---+++---+---+---+++---+---+---\n\n";
				for(size_t sub_row=0;sub_row<3;sub_row++){
					for(size_t col=0;col<9;col++){
						if(col==3 or col==6)
								cout <<"| ";
						for(size_t sub_col=0;sub_col<3;sub_col++){
							auto val=3*sub_row+sub_col;
							bool has=true;
							if(cells.size()>row*9+col)
								has=values[cells[row*9+col]][val];
							if(has)
								cout<<(val+1);
							else
								cout<<"_";}
						cout <<" ";}
					cout<<"\n\n";}}
			cout<<"\n\n---\n";
		};
		switch(port){
			case 0:
			if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==2)
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Add Cell")
							if(std::holds_alternative<UID>(v[1].body)){
								auto c=std::get<UID>(v[1].body);
								cells.push_back(c);
								if(cells.size()>81)
									crash_or_shutdown(true,id,m); // TODO: Elaborate
								for(int val=0;val<9;val++)
									values[c][val]=true;
								make_channel<BasicChannel>({id,1,c,1});
								make_channel<BasicChannel>({c,1,id,1});
								post(id,1,c,1,Message({"Query"}));
								break;
							}
					}
				if(v.size())
					if(std::holds_alternative<std::string>(v[0].body)){
						auto vv = std::get<std::string>(v[0].body);
						if(vv=="Shutting Down")
							break;
						if(vv=="Crashed") // Don't die on crashes, I need the logging
							break;}}
			crash_or_shutdown(true,id,m);
			break;
		case 1:
			if(std::holds_alternative<std::string>(m.body)){
				auto v = std::get<std::string>(m.body);
				if(v=="Display"){
					if(cells.size()!=81){
						crash_or_shutdown(true,id,m); // TODO: Elaborate
						break;}
					display();
					break;
				}
			}else if(std::holds_alternative<std::vector<Message>>(m.body)){
				auto v = std::get<std::vector<Message>>(m.body);
				if(v.size()==9){
					int changed=0;
					for(int val=0;val<9;val++)
						if(!std::holds_alternative<int>(v[val].body))
							crash_or_shutdown(true,id,m);
						else
							if((changed=(values[from][val]!=std::get<int>(v[val].body))))
								values[from][val]=std::get<int>(v[val].body);
					if(changed)
						display();
					break;
				}
			}
			crash_or_shutdown(true,id,m);
			break;
		default:
			crash_or_shutdown(true,id,m);
		}
	}
};
#endif
void sudoku_solver(int initial[9][9]){
	DB("Start  Sudoku Solver");
	std::vector<UID> cells;
	for(int row=0;row<9;row++)
		for(int col=0;col<9;col++){
			cells.push_back(spawn_properator<SudokuCell>());
			if(initial[row][col])
				post(0,0,cells.back(),1,Message({std::vector<Message>({Message({"Set"}),Message({initial[row][col]})})}));
		}

	auto displayer=spawn_properator<SudokuGridDisplay>();
	for(auto&c:cells)
		post(0,0,displayer,0,Message(std::vector<Message>({
							Message({"Add Cell"}),
							Message({c})})));

	#ifdef DEBUG
	auto displayerv=spawn_properator<SudokuGridVerboseDisplay>();
	for(auto&c:cells)
		post(0,0,displayerv,0,Message(std::vector<Message>({
							Message({"Add Cell"}),
							Message({c})})));
	#endif
	
	// make the onehots
	std::vector<UID> one_hots;
	DB("- Make the rows");
	for(int row=0;row<9;row++){
		one_hots.push_back(spawn_properator<SudokuValueAtMostOnce>());
		for(int col=0;col<9;col++)
			post(0,0,one_hots.back(),0,Message({std::vector<Message>({
								Message({"Add Cell"}),
								Message({cells[row*9+col]})})}));
		one_hots.push_back(spawn_properator<SudokuValueAtLeastOnce>());
		for(int col=0;col<9;col++)
			post(0,0,one_hots.back(),0,Message({std::vector<Message>({
								Message({"Add Cell"}),
								Message({cells[row*9+col]})})}));
	}

	DB("- Make the cols");
	for(int col=0;col<9;col++){
		one_hots.push_back(spawn_properator<SudokuValueAtMostOnce>());
		for(int row=0;row<9;row++)
			post(0,0,one_hots.back(),0,Message({std::vector<Message>({
								Message({"Add Cell"}),
								Message({cells[row*9+col]})})}));
		one_hots.push_back(spawn_properator<SudokuValueAtLeastOnce>());
		for(int row=0;row<9;row++)
			post(0,0,one_hots.back(),0,Message({std::vector<Message>({
								Message({"Add Cell"}),
								Message({cells[row*9+col]})})}));
	}

	DB("- Make the boxes");
	for(int box=0;box<9;box++){
		auto
			r_min=3*(box/3),
			c_min=3*(box%3);
		one_hots.push_back(spawn_properator<SudokuValueAtMostOnce>());
		for(int row=r_min;row<r_min+3;row++)
			for(int col=c_min;col<c_min+3;col++)
				post(0,0,one_hots.back(),0,Message({std::vector<Message>({
									Message({"Add Cell"}),
									Message({cells[row*9+col]})})}));
		one_hots.push_back(spawn_properator<SudokuValueAtLeastOnce>());
		for(int row=r_min;row<r_min+3;row++)
			for(int col=c_min;col<c_min+3;col++)
				post(0,0,one_hots.back(),0,Message({std::vector<Message>({
									Message({"Add Cell"}),
									Message({cells[row*9+col]})})}));
	}

	while(main_loop_step());
	post(0,0,displayer,1,Message({"Display"}));
	while(main_loop_step());

	for(auto&c:cells)
		crash_or_shutdown(false,c,Message({"Example Over"}));
	crash_or_shutdown(false,displayer,Message({"Example Over"}));
	// shutdown each that it made
	while(main_loop_step()); // Commenting this causes a strange bug.
	DB("Finish Sudoku Solver");
}
void sudoku_example(){
	// perform the four easy puzzles to watch this work.
	int grid1[9][9]={{5,0,0, 4,6,7, 3,0,9},
									 {9,0,3, 8,1,0, 4,2,7},
									 {1,7,4, 2,0,3, 0,0,0},
									 // -------------------
									 {2,3,1, 9,7,6, 8,5,4},
									 {8,5,7, 1,2,4, 0,9,0},
									 {4,9,6, 3,0,8, 1,7,2},
									 // -------------------
									 {0,0,0, 0,8,9, 2,6,0},
									 {7,8,2, 6,4,1, 0,0,5},
									 {0,1,0, 0,0,0, 7,0,8}};
	sudoku_solver(grid1);

	int grid2[9][9]={{5,0,0, 0,1,0, 0,0,4},
									 {2,7,4, 0,0,0, 6,0,0},
									 {0,8,0, 9,0,4, 0,0,0},
									 // -------------------
									 {8,1,0, 4,6,0, 3,0,2},
									 {0,0,2, 0,3,0, 1,0,0},
									 {7,0,6, 0,9,1, 0,5,8},
									 // -------------------
									 {0,0,0, 5,0,3, 0,1,0},
									 {0,0,5, 0,0,0, 9,2,7},
									 {1,0,0, 0,2,0, 0,0,3}};
	sudoku_solver(grid2);

	// Needs "only available" logic (e.g. There's only one available place for 6 in this row.)
	int grid3[9][9]={{0,8,0, 6,0,0, 0,1,0},
									 {0,0,0, 0,0,8, 2,5,6},
									 {0,0,1, 0,0,0, 0,0,0},
									 // -------------------
									 {0,0,0, 9,0,4, 6,0,3},
									 {0,0,9, 0,7,0, 5,0,0},
									 {4,0,7, 5,0,2, 0,0,0},
									 // -------------------
									 {0,0,0, 0,0,0, 8,0,0},
									 {7,1,3, 4,0,0, 0,0,0},
									 {0,5,0, 0,0,9, 0,3,0}};
	sudoku_solver(grid3);

	int grid4[9][9]={{0,0,0, 0,0,0, 0,0,0},
									 {8,3,0, 1,5,0, 0,7,4},
									 {0,2,0, 6,8,0, 0,9,0},
									 // -------------------
									 {0,7,0, 0,0,0, 1,3,0},
									 {0,4,0, 5,0,1, 0,0,7},
									 {9,0,3, 0,7,0, 0,4,0},
									 // -------------------
									 {7,8,6, 0,0,0, 0,1,2},
									 {0,0,1, 0,0,8, 0,0,0},
									 {0,0,4, 2,0,0, 0,0,0}};
	sudoku_solver(grid4);
}

int main(){
	printf("\n\nHello World Example\n");
	hello_example();
	printf("\n\nFactorial Example\n");
	factorial_example();
	printf("\n\nSudoku Example\n");
	sudoku_example();
}
