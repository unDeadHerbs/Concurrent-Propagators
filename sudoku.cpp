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
		bool changed=false;
		for(int i=0;i<9;i++)
			if(i!=v)
				if(can_be[i]){
					changed=true;
					can_be[i]=false;
				}
		if(changed)
			updated();}
};

struct AtMostOneEach:Propogator{
	// Only one cell can have a value.
	bool known[9]={0,0,0, 0,0,0, 0,0,0};
	SudokuCell* cells[9];
	AtMostOneEach(SudokuCell* a,SudokuCell* b,SudokuCell* c,
								SudokuCell* d,SudokuCell* e,SudokuCell* f,
								SudokuCell* g,SudokuCell* h,SudokuCell* i){
		cells[0]=a;cells[1]=b;cells[2]=c;
		cells[3]=d;cells[4]=e;cells[5]=f;
		cells[6]=g;cells[7]=h;cells[8]=i;
		for(int j=0;j<9;j++)
			cells[j]->watchers.push_back(this);
	}
	void alert(){
		for(int i=0;i<9;i++)
			if(cells[i]->is_found())
				if(!known[i]){
					known[i]=true;;
					for(int j=0;j<9;j++)
						if(!known[j])
							cells[j]->ban(cells[i]->val());}}
};

struct AtLeastOneEach:Propogator{
	// Each value must be in at least one cell.
	bool known[9]={0,0,0, 0,0,0, 0,0,0};	
	SudokuCell* cells[9];
	AtLeastOneEach(SudokuCell* a,SudokuCell* b,SudokuCell* c,
								SudokuCell* d,SudokuCell* e,SudokuCell* f,
								SudokuCell* g,SudokuCell* h,SudokuCell* i){
		cells[0]=a;cells[1]=b;cells[2]=c;
		cells[3]=d;cells[4]=e;cells[5]=f;
		cells[6]=g;cells[7]=h;cells[8]=i;
		for(int j=0;j<9;j++)
			cells[j]->watchers.push_back(this);
	}
	void alert(){
		for(int v=0;v<9;v++){
			if(known[v]) continue;
			int places=0;
			for(int c=0;c<9;c++)
				if(cells[c]->can_be[v])
					places++;
			known[v]=places==1;
			if(places==1)
				for(int c=0;c<9;c++)
					if(cells[c]->can_be[v])
						cells[c]->set(v);
		}
	}
};

struct PairLogic_Most:Propogator{
	// A pair of cells limited to 2 values must have them.
	SudokuCell* cells[9];
	PairLogic_Most(SudokuCell* a,SudokuCell* b,SudokuCell* c,
								 SudokuCell* d,SudokuCell* e,SudokuCell* f,
								 SudokuCell* g,SudokuCell* h,SudokuCell* i){
		cells[0]=a;cells[1]=b;cells[2]=c;
		cells[3]=d;cells[4]=e;cells[5]=f;
		cells[6]=g;cells[7]=h;cells[8]=i;
		for(int j=0;j<9;j++)
			cells[j]->watchers.push_back(this);
	}
	void alert(){
		for(int c1=0;c1<9;c1++)
			if(!cells[c1]->is_found())
				for(int c2=0;c2<9;c2++)
					if(c1!=c2)
						if(!cells[c2]->is_found()){
							int found_vs=0;
							// if c1 and c2 share 2 values
							for(int v=0;v<9;v++)
								if(cells[c1]->can_be[v] or cells[c2]->can_be[v])
									found_vs++;
							// the rest can't have those two
							if(found_vs==2)
								for(int c3=0;c3<9;c3++)
									if(c3!=c1 and c3!=c2)
										for(int v=0;v<9;v++)
											if(cells[c1]->can_be[v] or cells[c2]->can_be[v])
												cells[c3]->ban(v);}}
};

struct PairLogic_Least:Propogator{
	// A pair of values limited to two cells must be in them.
	int found[9]={0,0,0, 0,0,0, 0,0,0};
	SudokuCell* cells[9];
	PairLogic_Least(SudokuCell* a,SudokuCell* b,SudokuCell* c,
									SudokuCell* d,SudokuCell* e,SudokuCell* f,
									SudokuCell* g,SudokuCell* h,SudokuCell* i){
		cells[0]=a;cells[1]=b;cells[2]=c;
		cells[3]=d;cells[4]=e;cells[5]=f;
		cells[6]=g;cells[7]=h;cells[8]=i;
		for(int j=0;j<9;j++)
			cells[j]->watchers.push_back(this);
	}
	void alert(){
		for(int c=0;c<9;c++)
			if(cells[c]->is_found())
				found[cells[c]->val()]=true;
		
		for(int v1=0;v1<9;v1++)
			if(!found[v1])
				for(int v2=0;v2<9;v2++)
					if(!found[v2])
						if(v1!=v2){
							// if only two cells can contain
							int found_c=0;
							for(int c=0;c<9;c++)
								if(cells[c]->can_be[v1] or cells[c]->can_be[v2])
									found_c++;
							if(found_c==2)
								// ban all other values from them.
								for(int c=0;c<9;c++)
									if(cells[c]->can_be[v1] or cells[c]->can_be[v2])
										for(int v3=0;v3<9;v3++)
											if(v3!=v1 and v3!=v2)
												cells[c]->ban(v3);
						}}
						
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
	int solved=0;
	SudokuCell(* grid)[81];
	GridPrinter(SudokuCell(* g)[81]):grid(g){
		alert();
		for(int i=0;i<81;i++)
			(*grid)[i].watchers.push_back(this);}
	void alert(){
		int cur_solve = 0;
		for(int row=0;row<9;row++)
			for(int col=0;col<9;col++)
				if((*grid)[row*9+col].is_found())
					cur_solve++;
		if(cur_solve!=solved)
			grid_display(*grid);
		solved=cur_solve;
	}
};

bool sudoku_solver(int initial[9][9]){
	SudokuCell grid[81];

	// The basic "only one of each" requirement
	AtMostOneEach rcb_most[27]={
		// rows
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[0*9+6],&grid[0*9+7],&grid[0*9+8]},
		{&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8]},
		{&grid[2*9+0],&grid[2*9+1],&grid[2*9+2],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[3*9+6],&grid[3*9+7],&grid[3*9+8]},
		{&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8]},
		{&grid[5*9+0],&grid[5*9+1],&grid[5*9+2],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[6*9+6],&grid[6*9+7],&grid[6*9+8]},
		{&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8]},
		{&grid[8*9+0],&grid[8*9+1],&grid[8*9+2],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]},

		// columns
		{&grid[0*9+0],&grid[1*9+0],&grid[2*9+0],&grid[3*9+0],&grid[4*9+0],&grid[5*9+0],&grid[6*9+0],&grid[7*9+0],&grid[8*9+0]},
		{&grid[0*9+1],&grid[1*9+1],&grid[2*9+1],&grid[3*9+1],&grid[4*9+1],&grid[5*9+1],&grid[6*9+1],&grid[7*9+1],&grid[8*9+1]},
		{&grid[0*9+2],&grid[1*9+2],&grid[2*9+2],&grid[3*9+2],&grid[4*9+2],&grid[5*9+2],&grid[6*9+2],&grid[7*9+2],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[1*9+3],&grid[2*9+3],&grid[3*9+3],&grid[4*9+3],&grid[5*9+3],&grid[6*9+3],&grid[7*9+3],&grid[8*9+3]},
		{&grid[0*9+4],&grid[1*9+4],&grid[2*9+4],&grid[3*9+4],&grid[4*9+4],&grid[5*9+4],&grid[6*9+4],&grid[7*9+4],&grid[8*9+4]},
		{&grid[0*9+5],&grid[1*9+5],&grid[2*9+5],&grid[3*9+5],&grid[4*9+5],&grid[5*9+5],&grid[6*9+5],&grid[7*9+5],&grid[8*9+5]},

		{&grid[0*9+6],&grid[1*9+6],&grid[2*9+6],&grid[3*9+6],&grid[4*9+6],&grid[5*9+6],&grid[6*9+6],&grid[7*9+6],&grid[8*9+6]},
		{&grid[0*9+7],&grid[1*9+7],&grid[2*9+7],&grid[3*9+7],&grid[4*9+7],&grid[5*9+7],&grid[6*9+7],&grid[7*9+7],&grid[8*9+7]},
		{&grid[0*9+8],&grid[1*9+8],&grid[2*9+8],&grid[3*9+8],&grid[4*9+8],&grid[5*9+8],&grid[6*9+8],&grid[7*9+8],&grid[8*9+8]},
	
		// boxes
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[2*9+0],&grid[2*9+1],&grid[2*9+2]},
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[5*9+0],&grid[5*9+1],&grid[5*9+2]},
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[8*9+0],&grid[8*9+1],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5]},
		{&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5]},
		{&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5]},
		
		{&grid[0*9+6],&grid[0*9+7],&grid[0*9+8],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		{&grid[3*9+6],&grid[3*9+7],&grid[3*9+8],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		{&grid[6*9+6],&grid[6*9+7],&grid[6*9+8],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
	(void)rcb_most;

		AtLeastOneEach rcb_least[27]={
		// rows
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[0*9+6],&grid[0*9+7],&grid[0*9+8]},
		{&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8]},
		{&grid[2*9+0],&grid[2*9+1],&grid[2*9+2],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[3*9+6],&grid[3*9+7],&grid[3*9+8]},
		{&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8]},
		{&grid[5*9+0],&grid[5*9+1],&grid[5*9+2],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[6*9+6],&grid[6*9+7],&grid[6*9+8]},
		{&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8]},
		{&grid[8*9+0],&grid[8*9+1],&grid[8*9+2],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]},

		// columns
		{&grid[0*9+0],&grid[1*9+0],&grid[2*9+0],&grid[3*9+0],&grid[4*9+0],&grid[5*9+0],&grid[6*9+0],&grid[7*9+0],&grid[8*9+0]},
		{&grid[0*9+1],&grid[1*9+1],&grid[2*9+1],&grid[3*9+1],&grid[4*9+1],&grid[5*9+1],&grid[6*9+1],&grid[7*9+1],&grid[8*9+1]},
		{&grid[0*9+2],&grid[1*9+2],&grid[2*9+2],&grid[3*9+2],&grid[4*9+2],&grid[5*9+2],&grid[6*9+2],&grid[7*9+2],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[1*9+3],&grid[2*9+3],&grid[3*9+3],&grid[4*9+3],&grid[5*9+3],&grid[6*9+3],&grid[7*9+3],&grid[8*9+3]},
		{&grid[0*9+4],&grid[1*9+4],&grid[2*9+4],&grid[3*9+4],&grid[4*9+4],&grid[5*9+4],&grid[6*9+4],&grid[7*9+4],&grid[8*9+4]},
		{&grid[0*9+5],&grid[1*9+5],&grid[2*9+5],&grid[3*9+5],&grid[4*9+5],&grid[5*9+5],&grid[6*9+5],&grid[7*9+5],&grid[8*9+5]},

		{&grid[0*9+6],&grid[1*9+6],&grid[2*9+6],&grid[3*9+6],&grid[4*9+6],&grid[5*9+6],&grid[6*9+6],&grid[7*9+6],&grid[8*9+6]},
		{&grid[0*9+7],&grid[1*9+7],&grid[2*9+7],&grid[3*9+7],&grid[4*9+7],&grid[5*9+7],&grid[6*9+7],&grid[7*9+7],&grid[8*9+7]},
		{&grid[0*9+8],&grid[1*9+8],&grid[2*9+8],&grid[3*9+8],&grid[4*9+8],&grid[5*9+8],&grid[6*9+8],&grid[7*9+8],&grid[8*9+8]},
	
		// boxes
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[2*9+0],&grid[2*9+1],&grid[2*9+2]},
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[5*9+0],&grid[5*9+1],&grid[5*9+2]},
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[8*9+0],&grid[8*9+1],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5]},
		{&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5]},
		{&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5]},
		
		{&grid[0*9+6],&grid[0*9+7],&grid[0*9+8],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		{&grid[3*9+6],&grid[3*9+7],&grid[3*9+8],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		{&grid[6*9+6],&grid[6*9+7],&grid[6*9+8],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
		(void)rcb_least;

		PairLogic_Most rcb_pair_most[27]={
		// rows
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[0*9+6],&grid[0*9+7],&grid[0*9+8]},
		{&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8]},
		{&grid[2*9+0],&grid[2*9+1],&grid[2*9+2],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[3*9+6],&grid[3*9+7],&grid[3*9+8]},
		{&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8]},
		{&grid[5*9+0],&grid[5*9+1],&grid[5*9+2],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[6*9+6],&grid[6*9+7],&grid[6*9+8]},
		{&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8]},
		{&grid[8*9+0],&grid[8*9+1],&grid[8*9+2],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]},

		// columns
		{&grid[0*9+0],&grid[1*9+0],&grid[2*9+0],&grid[3*9+0],&grid[4*9+0],&grid[5*9+0],&grid[6*9+0],&grid[7*9+0],&grid[8*9+0]},
		{&grid[0*9+1],&grid[1*9+1],&grid[2*9+1],&grid[3*9+1],&grid[4*9+1],&grid[5*9+1],&grid[6*9+1],&grid[7*9+1],&grid[8*9+1]},
		{&grid[0*9+2],&grid[1*9+2],&grid[2*9+2],&grid[3*9+2],&grid[4*9+2],&grid[5*9+2],&grid[6*9+2],&grid[7*9+2],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[1*9+3],&grid[2*9+3],&grid[3*9+3],&grid[4*9+3],&grid[5*9+3],&grid[6*9+3],&grid[7*9+3],&grid[8*9+3]},
		{&grid[0*9+4],&grid[1*9+4],&grid[2*9+4],&grid[3*9+4],&grid[4*9+4],&grid[5*9+4],&grid[6*9+4],&grid[7*9+4],&grid[8*9+4]},
		{&grid[0*9+5],&grid[1*9+5],&grid[2*9+5],&grid[3*9+5],&grid[4*9+5],&grid[5*9+5],&grid[6*9+5],&grid[7*9+5],&grid[8*9+5]},

		{&grid[0*9+6],&grid[1*9+6],&grid[2*9+6],&grid[3*9+6],&grid[4*9+6],&grid[5*9+6],&grid[6*9+6],&grid[7*9+6],&grid[8*9+6]},
		{&grid[0*9+7],&grid[1*9+7],&grid[2*9+7],&grid[3*9+7],&grid[4*9+7],&grid[5*9+7],&grid[6*9+7],&grid[7*9+7],&grid[8*9+7]},
		{&grid[0*9+8],&grid[1*9+8],&grid[2*9+8],&grid[3*9+8],&grid[4*9+8],&grid[5*9+8],&grid[6*9+8],&grid[7*9+8],&grid[8*9+8]},
	
		// boxes
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[2*9+0],&grid[2*9+1],&grid[2*9+2]},
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[5*9+0],&grid[5*9+1],&grid[5*9+2]},
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[8*9+0],&grid[8*9+1],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5]},
		{&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5]},
		{&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5]},
		
		{&grid[0*9+6],&grid[0*9+7],&grid[0*9+8],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		{&grid[3*9+6],&grid[3*9+7],&grid[3*9+8],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		{&grid[6*9+6],&grid[6*9+7],&grid[6*9+8],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
		(void)rcb_pair_most;

		PairLogic_Least rcb_pair_least[27]={
		// rows
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[0*9+6],&grid[0*9+7],&grid[0*9+8]},
		{&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8]},
		{&grid[2*9+0],&grid[2*9+1],&grid[2*9+2],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[3*9+6],&grid[3*9+7],&grid[3*9+8]},
		{&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8]},
		{&grid[5*9+0],&grid[5*9+1],&grid[5*9+2],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[6*9+6],&grid[6*9+7],&grid[6*9+8]},
		{&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8]},
		{&grid[8*9+0],&grid[8*9+1],&grid[8*9+2],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]},

		// columns
		{&grid[0*9+0],&grid[1*9+0],&grid[2*9+0],&grid[3*9+0],&grid[4*9+0],&grid[5*9+0],&grid[6*9+0],&grid[7*9+0],&grid[8*9+0]},
		{&grid[0*9+1],&grid[1*9+1],&grid[2*9+1],&grid[3*9+1],&grid[4*9+1],&grid[5*9+1],&grid[6*9+1],&grid[7*9+1],&grid[8*9+1]},
		{&grid[0*9+2],&grid[1*9+2],&grid[2*9+2],&grid[3*9+2],&grid[4*9+2],&grid[5*9+2],&grid[6*9+2],&grid[7*9+2],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[1*9+3],&grid[2*9+3],&grid[3*9+3],&grid[4*9+3],&grid[5*9+3],&grid[6*9+3],&grid[7*9+3],&grid[8*9+3]},
		{&grid[0*9+4],&grid[1*9+4],&grid[2*9+4],&grid[3*9+4],&grid[4*9+4],&grid[5*9+4],&grid[6*9+4],&grid[7*9+4],&grid[8*9+4]},
		{&grid[0*9+5],&grid[1*9+5],&grid[2*9+5],&grid[3*9+5],&grid[4*9+5],&grid[5*9+5],&grid[6*9+5],&grid[7*9+5],&grid[8*9+5]},

		{&grid[0*9+6],&grid[1*9+6],&grid[2*9+6],&grid[3*9+6],&grid[4*9+6],&grid[5*9+6],&grid[6*9+6],&grid[7*9+6],&grid[8*9+6]},
		{&grid[0*9+7],&grid[1*9+7],&grid[2*9+7],&grid[3*9+7],&grid[4*9+7],&grid[5*9+7],&grid[6*9+7],&grid[7*9+7],&grid[8*9+7]},
		{&grid[0*9+8],&grid[1*9+8],&grid[2*9+8],&grid[3*9+8],&grid[4*9+8],&grid[5*9+8],&grid[6*9+8],&grid[7*9+8],&grid[8*9+8]},
	
		// boxes
		{&grid[0*9+0],&grid[0*9+1],&grid[0*9+2],&grid[1*9+0],&grid[1*9+1],&grid[1*9+2],&grid[2*9+0],&grid[2*9+1],&grid[2*9+2]},
		{&grid[3*9+0],&grid[3*9+1],&grid[3*9+2],&grid[4*9+0],&grid[4*9+1],&grid[4*9+2],&grid[5*9+0],&grid[5*9+1],&grid[5*9+2]},
		{&grid[6*9+0],&grid[6*9+1],&grid[6*9+2],&grid[7*9+0],&grid[7*9+1],&grid[7*9+2],&grid[8*9+0],&grid[8*9+1],&grid[8*9+2]},
		
		{&grid[0*9+3],&grid[0*9+4],&grid[0*9+5],&grid[1*9+3],&grid[1*9+4],&grid[1*9+5],&grid[2*9+3],&grid[2*9+4],&grid[2*9+5]},
		{&grid[3*9+3],&grid[3*9+4],&grid[3*9+5],&grid[4*9+3],&grid[4*9+4],&grid[4*9+5],&grid[5*9+3],&grid[5*9+4],&grid[5*9+5]},
		{&grid[6*9+3],&grid[6*9+4],&grid[6*9+5],&grid[7*9+3],&grid[7*9+4],&grid[7*9+5],&grid[8*9+3],&grid[8*9+4],&grid[8*9+5]},
		
		{&grid[0*9+6],&grid[0*9+7],&grid[0*9+8],&grid[1*9+6],&grid[1*9+7],&grid[1*9+8],&grid[2*9+6],&grid[2*9+7],&grid[2*9+8]},
		{&grid[3*9+6],&grid[3*9+7],&grid[3*9+8],&grid[4*9+6],&grid[4*9+7],&grid[4*9+8],&grid[5*9+6],&grid[5*9+7],&grid[5*9+8]},
		{&grid[6*9+6],&grid[6*9+7],&grid[6*9+8],&grid[7*9+6],&grid[7*9+7],&grid[7*9+8],&grid[8*9+6],&grid[8*9+7],&grid[8*9+8]}};
		(void)rcb_pair_least;

	// Add the values
	for(int row=0;row<9;row++)
		for(int col=0;col<9;col++)
			if(initial[row][col])
				grid[row*9+col].set(initial[row][col]-1);
	
	// Show the state
	GridPrinter gp(&grid);	

	// Check if solved
	for(int row=0;row<9;row++)
		for(int col=0;col<9;col++)
			if(!grid[row*9+col].is_found())
				return false;
	return true;
}

int main(){
	StringCell sc;
	AutoPrinter ap(&sc);
	sc.set("hello world\n");

	// Only naked singles required
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

	
	if(sudoku_solver(grid1))
		std::cout<<"Passed 1"<<std::endl;
	else
		std::cout<<"Failed 1"<<std::endl;
	

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

	if(sudoku_solver(grid2))
		std::cout<<"Passed 2"<<std::endl;
	else
		std::cout<<"Failed 2"<<std::endl;

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

	if(sudoku_solver(grid3))
		std::cout<<"Passed 3"<<std::endl;
	else
		std::cout<<"Failed 3"<<std::endl;

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

	if(sudoku_solver(grid4))
		std::cout<<"Passed 4"<<std::endl;
	else
		std::cout<<"Failed 4"<<std::endl;

	// Pair Logic
	int grid5[9][9]={{3,0,0, 8,0,0, 0,0,0},
									 {0,6,0, 0,0,2, 5,0,0},
									 {8,0,1, 0,3,0, 0,2,0},
									 // -------------------
									 {0,9,5, 0,0,3, 0,0,2},
									 {0,2,0, 5,0,4, 0,6,0},
									 {6,0,0, 2,0,0, 7,5,0},
									 // -------------------
									 {0,1,0, 0,2,0, 8,0,6},
									 {0,0,4, 3,0,0, 0,9,0},
									 {0,0,0, 0,0,7, 0,0,5}};

	if(sudoku_solver(grid5))
		std::cout<<"Passed 5"<<std::endl;
	else
		std::cout<<"Failed 5"<<std::endl;

	/*
	int grid6[9][9]={{8,0,1, 0,0,6, 0,9,4},
									 {3,0,0, 0,0,9, 0,8,0},
									 {9,7,0, 0,8,0, 5,0,0},
									 // -------------------
									 {5,4,7, 0,6,2, 0,3,0},
									 {6,3,2, 0,0,0, 0,5,0},
									 {1,9,8, 3,7,5, 2,4,6},
									 // -------------------
									 {0,8,3, 6,2,0, 9,1,5},
									 {0,6,5, 1,9,8, 0,0,0},
									 {2,1,9, 5,0,0, 0,0,8}};

	if(sudoku_solver(grid6))
		std::cout<<"Passed 6"<<std::endl;
	else
		std::cout<<"Failed 6"<<std::endl;

	
	// Harder
	int grid99[9][9]={{3,0,0, 0,2,0, 0,0,0},
									 {9,6,0, 0,7,0, 0,0,0},
									 {0,0,0, 4,0,1, 3,8,0},
									 // -------------------
									 {0,2,0, 8,0,0, 0,4,0},
									 {0,0,0, 0,0,0, 0,0,0},
									 {6,4,0, 0,1,0, 0,3,0},
									 // -------------------
									 {0,0,3, 0,0,0, 7,0,0},
									 {2,0,0, 0,0,7, 8,0,9},
									 {0,0,0, 0,0,0, 0,2,0}};

	if(sudoku_solver(grid99))
		std::cout<<"Passed 99"<<std::endl;
	else
		std::cout<<"Failed 99"<<std::endl;

	// Template
	int grid999[9][9]={{0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0},
										 // -------------------
										 {0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0},
										 // -------------------
										 {0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0},
										 {0,0,0, 0,0,0, 0,0,0}};

	if(sudoku_solver(grid999))
		std::cout<<"Passed 999"<<std::endl;
	else
		std::cout<<"Failed 999"<<std::endl;
	//*/
}
