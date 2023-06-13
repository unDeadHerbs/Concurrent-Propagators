#include "properator.hpp"
#include <stdio.h>
#include <map>

// for operator ""s
using namespace std::string_literals;

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

void hello_example(){
	auto s_hold = spawn_properator<StringHolder>();
	auto printer = spawn_properator<MessageLogger>();
	make_channel<BasicChannel>({s_hold,1,printer,1});
	post(0,0,s_hold,1,Message({"Hello, World!"}));
	while(main_loop_step());
	crash_or_shutdown(false,printer,Message({"Example Over"}));
	while(main_loop_step());
	crash_or_shutdown(false,s_hold,Message({"Example Over"}));
	while(main_loop_step());
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
	auto fact = spawn_properator<FactorialCalculator>();
	auto printer = spawn_properator<MessageLogger>();
	make_channel<BasicChannel>({fact,1,printer,1});
	post(0,0,fact,1,Message({5}));
	while(main_loop_step());
	crash_or_shutdown(false,printer,Message({"Example Over"}));
	while(main_loop_step());
	crash_or_shutdown(false,fact,Message({"Example Over"}));
	while(main_loop_step());
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
					printf("\n");
					for(int row=0;row<9;row++){
						if(row==3 or row==6)
							printf("---+---+---\n");
						for(int col=0;col<9;col++){
							auto val=values[cells[row*9+col]];
							if(col==3 or col==6)
								printf("|");
							if(val)
								printf("%d",val);
							else
								printf("_");}
						printf("\n");}
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
			printf("\n");
			for(size_t row=0;row<9;row++){
				if(row==3 or row==6)
					printf("---+---+---+++---+---+---+++---+---+---\n\n");
				for(size_t sub_row=0;sub_row<3;sub_row++){
					for(size_t col=0;col<9;col++){
						if(col==3 or col==6)
							printf("| ");
						for(size_t sub_col=0;sub_col<3;sub_col++){
							auto val=3*sub_row+sub_col;
							bool has=true;
							if(cells.size()>row*9+col)
								has=values[cells[row*9+col]][val];
							if(has)
								printf("%d",val+1);
							else
								printf("_");}
						printf(" ");}
					printf("\n\n");}}
			printf("\n\n---\n");
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
	//printf("- Make the rows\n");
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

	//printf("- Make the cols\n");
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

	//printf("- Make the boxes\n");
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

	while(main_loop_step());
}
void sudoku_example(){
	// Perform four easy puzzles to watch this work.
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

