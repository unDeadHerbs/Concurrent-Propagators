#include <iostream>
#include <string>
#include <vector>

struct Propogator{virtual void alert()=0;};
struct Cell{
	std::vector<Propogator*> watchers;
	void updated(){for(auto w:watchers)w->alert();}
};

struct StringCell:Cell{
	std::string value;
	void set(std::string v){value=v;updated();}
};
struct AutoPrinter:Propogator{
	StringCell* watched;
	AutoPrinter(StringCell*w):watched(w){watched->watchers.push_back(this);}
	void alert(){std::cout<<watched->value<<std::flush;}
};

int main(){
	StringCell sc;
	AutoPrinter ap(&sc);
	sc.set("hello world\n");
}
