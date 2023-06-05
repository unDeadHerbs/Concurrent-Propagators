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
	StringCell* watching;
	AutoPrinter(StringCell*w):watching(w){watching->watchers.push_back(this);}
	void alert(){std::cout<<watching->value<<std::flush;}
};

struct SudokuCell:Cell{
	bool can_be[9]={1,1,1, 1,1,1, 1,1,1};
	void ban(int n){if(can_be[n]){can_be[n]=false;updated();}}
	bool is_found(){
		int vals=0;
		for(int i=0;i<9;i++)
			vals+=can_be[i];
		return vals==1;
	}
	int val(){ // only valid if settled
		for(int i=0;i<9;i++)
			if(can_be[i])
				return i;
		return -1;}
	void set(int v){
		for(int i=0;i<9;i++)
			if(i!=v)
				can_be[i]=false;
		updated();}
};
struct SudokuOneHot:Propogator{
	// The constraint that only one can have a value
	SudokuCell* cells[9];
	SudokuOneHot(SudokuCell* a,SudokuCell* b,SudokuCell* c,
							 SudokuCell* d,SudokuCell* e,SudokuCell* f,
							 SudokuCell* g,SudokuCell* h,SudokuCell* i){
		cells[0]=a;cells[1]=b;cells[2]=c;
		cells[3]=d;cells[4]=e;cells[5]=f;
		cells[6]=g;cells[7]=h;cells[8]=i;
		for(int j=0;j<9;j++)
			cells[j]->watchers.push_back(this);
	}
	void alert(){
		// TODO: This is very inefficient; but, readable and correct.
		for(int i=0;i<9;i++)
			if(cells[i]->is_found())
				for(int j=0;j<9;j++)
					if(i!=j)
						cells[j]->ban(cells[i]->val());}
};

void grid_display(SudokuCell grid[81]){
	// TODO: Use curses
	std::cout<<"\n";
	for(int row=0;row<9;row++){
		if(row==3 or row==6)
			std::cout<<"---+---+---\n";
		for(int col=0;col<9;col++){
			if(col==3 or col==6)
				std::cout <<"|";
			if(grid[row*9+col].is_found())
				std::cout<<grid[row*9+col].val()+1;
			else
				std::cout<<"_";}
		std::cout<<"\n";}}

struct GridPrinter:Propogator{
	SudokuCell(* grid)[81];
	GridPrinter(SudokuCell(* g)[81]):grid(g){
		alert();
		for(int i=0;i<81;i++)
			(*grid)[i].watchers.push_back(this);}
	void alert(){grid_display(*grid);}
};

int main(){
	StringCell sc;
	AutoPrinter ap(&sc);
	sc.set("hello world\n");

	SudokuCell grid[81];
	GridPrinter gp(&grid);
	SudokuOneHot rows[9]={{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[0*9+6],&grid[0*9+7],&grid[0*9+8]},
												{&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8]},
												{&grid[2*9+0],&grid[2*9+1],&grid[2*9+2],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},

												{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[3*9+6],&grid[3*9+7],&grid[3*9+8]},
												{&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8]},
												{&grid[5*9+0],&grid[5*9+1],&grid[5*9+2],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},

												{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[6*9+6],&grid[6*9+7],&grid[6*9+8]},
												{&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8]},
												{&grid[8*9+0],&grid[8*9+1],&grid[8*9+2],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
	(void)rows; // Remove unused warning.
	SudokuOneHot cols[9]={{&grid[0*9+0],&grid[1*9+0],&grid[2*9+0],&grid[3*9+0],&grid[4*9+0],&grid[5*9+0],&grid[6*9+0],&grid[7*9+0],&grid[8*9+0]},
												{&grid[0*9+1],&grid[1*9+1],&grid[2*9+1],&grid[3*9+1],&grid[4*9+1],&grid[5*9+1],&grid[6*9+1],&grid[7*9+1],&grid[8*9+1]},
												{&grid[0*9+2],&grid[1*9+2],&grid[2*9+2],&grid[3*9+2],&grid[4*9+2],&grid[5*9+2],&grid[6*9+2],&grid[7*9+2],&grid[8*9+2]},

												{&grid[0*9+3],&grid[1*9+3],&grid[2*9+3],&grid[3*9+3],&grid[4*9+3],&grid[5*9+3],&grid[6*9+3],&grid[7*9+3],&grid[8*9+3]},
												{&grid[0*9+4],&grid[1*9+4],&grid[2*9+4],&grid[3*9+4],&grid[4*9+4],&grid[5*9+4],&grid[6*9+4],&grid[7*9+4],&grid[8*9+4]},
												{&grid[0*9+5],&grid[1*9+5],&grid[2*9+5],&grid[3*9+5],&grid[4*9+5],&grid[5*9+5],&grid[6*9+5],&grid[7*9+5],&grid[8*9+5]},

												{&grid[0*9+6],&grid[1*9+6],&grid[2*9+6],&grid[3*9+6],&grid[4*9+6],&grid[5*9+6],&grid[6*9+6],&grid[7*9+6],&grid[8*9+6]},
												{&grid[0*9+7],&grid[1*9+7],&grid[2*9+7],&grid[3*9+7],&grid[4*9+7],&grid[5*9+7],&grid[6*9+7],&grid[7*9+7],&grid[8*9+7]},
												{&grid[0*9+8],&grid[1*9+8],&grid[2*9+8],&grid[3*9+8],&grid[4*9+8],&grid[5*9+8],&grid[6*9+8],&grid[7*9+8],&grid[8*9+8]}};
	(void)cols;
	SudokuOneHot boxes[9]={{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[2*9+0],&grid[2*9+1],&grid[2*9+2]},
												 {&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[5*9+0],&grid[5*9+1],&grid[5*9+2]},
												 {&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[8*9+0],&grid[8*9+1],&grid[8*9+2]},
												 
												 {&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5]},
												 {&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5]},
												 {&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5]},
												 
												 {&grid[0*9+6],&grid[0*9+7],&grid[0*9+8],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
												 {&grid[3*9+6],&grid[3*9+7],&grid[3*9+8],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
												 {&grid[6*9+6],&grid[6*9+7],&grid[6*9+8],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
	(void)boxes;
	// 5__|467|3_9
	// 9_3|81_|427
	// 174|2_3|___
	// ---+---+---
	// 231|976|854
	// 857|124|_9_
	// 496|3_8|172
	// ---+---+---
	// ___|_89|26_
	// 782|641|__5
	// _1_|___|7_8
	int c=0;
	grid[c++].set(5-1);
	c++; // blank
	c++; // blank
	grid[c++].set(4-1);
	grid[c++].set(6-1);
	grid[c++].set(7-1);
	grid[c++].set(3-1);
	c++; // blank
	grid[c++].set(9-1);
	grid[c++].set(9-1);
	c++; // blank
	grid[c++].set(3-1);
	grid[c++].set(8-1);
	grid[c++].set(1-1);
	c++; // blank
	grid[c++].set(4-1);
	grid[c++].set(2-1);
	grid[c++].set(7-1);
	grid[c++].set(1-1);
	grid[c++].set(7-1);
	grid[c++].set(4-1);
	grid[c++].set(2-1);
	c++; // blank
	grid[c++].set(3-1);
	c++; // blank
	c++; // blank
	c++; // blank
	grid[c++].set(2-1);
	grid[c++].set(3-1);
	grid[c++].set(1-1);
	grid[c++].set(9-1);
	grid[c++].set(7-1);
	grid[c++].set(6-1);
	grid[c++].set(8-1);
	grid[c++].set(5-1);
	grid[c++].set(4-1);
	grid[c++].set(8-1);
	grid[c++].set(5-1);
	grid[c++].set(7-1);
	grid[c++].set(1-1);
	grid[c++].set(2-1);
	grid[c++].set(4-1);
	c++; // blank
	grid[c++].set(9-1);
	c++; // blank
	grid[c++].set(4-1);
	grid[c++].set(9-1);
	grid[c++].set(6-1);
	grid[c++].set(3-1);
	c++; // blank
	grid[c++].set(8-1);
	grid[c++].set(1-1);
	grid[c++].set(7-1);
	grid[c++].set(2-1);
	c++; // blank
	c++; // blank
	c++; // blank
	c++; // blank
	grid[c++].set(8-1);
	grid[c++].set(9-1);
	grid[c++].set(2-1);
	grid[c++].set(6-1);
	c++; // blank
	grid[c++].set(7-1);
	grid[c++].set(8-1);
	grid[c++].set(2-1);
	grid[c++].set(6-1);
	grid[c++].set(4-1);
	grid[c++].set(1-1);
	c++; // blank
	c++; // blank
	grid[c++].set(5-1);
	c++; // blank
	grid[c++].set(1-1);
	c++; // blank
	c++; // blank
	c++; // blank
	c++; // blank
	grid[c++].set(7-1);
	c++; // blank
	grid[c++].set(8-1);
}
