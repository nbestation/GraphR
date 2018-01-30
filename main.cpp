#include<cassert>
#include<cstdio>
#include<cstring>
#include<string>
#include<vector>
#include<iostream>
#include<algorithm>
#include<sstream>
#include<unordered_map>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>

using namespace std;

int processRaw(char* mmap_ptr, size_t begin, size_t last, vector<struct edge>&raw_edge);
inline double DiffTime(struct timespec time1,struct timespec time2){return time2.tv_sec-time1.tv_sec+(time2.tv_nsec-time1.tv_nsec)*1e-9;}

struct edge{
    uint64_t src;
    uint64_t dst;
//    edge (uint64_t s, uint64_t d) : src(s), dst(d) {}
};

struct degree{
    int idx;
    int dgr;
};

inline bool comp_src(const struct edge &a,const struct edge &b){return a.src>b.src;}
inline bool comp_dst(const struct edge &a,const struct edge &b){return a.dst>b.dst;}
inline bool comp_dgr(const struct degree &a, const struct degree &b){return a.dgr>b.dgr;}

int main(int argc, char* argv[]){
	
    int opt;
    string file_name;
    bool sort_vertex = false;
    string help_string = string("Usage: ")+argv[0]+" -r RawFile [-s whetherSort]\n";
    while ((opt = getopt(argc, argv, "r:s")) != -1){
        switch  (opt){
            case 'r':
                file_name = optarg;
                cout << "Open " << file_name << endl;
                break;
            case 's':
                sort_vertex = true;
                cout << "Sorted vertex\n";
                break;
            default:
                cout << help_string;
                return EXIT_FAILURE;
        }
    }
    if (file_name.empty()){
        cout << help_string;
        return -1;
    }
    
	vector<struct edge> raw_edge(0);
	int raw_fd;
	struct stat sb;
	char* raw_mmap_ptr;
	size_t raw_mmap_length;
	size_t txt_buf_th = 1024 * 1024;
	assert((raw_fd = open(file_name.c_str(),O_RDONLY))!=-1);
	assert(fstat(raw_fd,&sb)!=-1);
	raw_mmap_length = sb.st_size;
    
    //struct timespec time0,time1;

	assert((raw_mmap_ptr = (char*)mmap(nullptr,raw_mmap_length,PROT_READ,MAP_SHARED,raw_fd,/*offset=*/0))!=MAP_FAILED);
	size_t last_pos = 0;
    int max = 0;
    for(size_t i=0;i<raw_mmap_length || last_pos<i;++i){
		if((i-last_pos>=txt_buf_th && (raw_mmap_ptr[i]=='\n'||raw_mmap_ptr[i]=='\r'))||(i>raw_mmap_length)){
			int max_temp = processRaw(raw_mmap_ptr,last_pos,i, raw_edge);
			last_pos=i+1;
            max = max > max_temp ? max : max_temp;
        }
	}
    cout << "total edges:" << raw_edge.size() << endl; 
    cout << "max id: " << max << endl;


    //sort
    vector<int> mapping(max+1);
    for (int i=0; i<=max; i++) mapping[i] = i;
    if (sort_vertex){
        cout << "remapping\n";
        vector<struct degree> vertex_degree(max+1);
        for (int i=0; i<=max;i++){
            vertex_degree[i].idx = i;
            vertex_degree[i].dgr = 0;
        }
        for (int i=0; i<raw_edge.size(); i++){
            vertex_degree[raw_edge[i].src].dgr++;
        }
        sort(vertex_degree.begin(), vertex_degree.end(), comp_dgr);
        for (int i=0; i<=max; i++) mapping[i] = vertex_degree[i].idx;
    }

    unordered_map<uint64_t, int> count;
    for (int i=0; i<raw_edge.size();i++){
        uint64_t src_idx = mapping[raw_edge[i].src]/4;
        uint64_t dst_idx = mapping[raw_edge[i].dst]/4;
        uint64_t idx = (src_idx<<32)+dst_idx;
        auto it = count.find(idx);
        if (it != count.end()){
            count[idx]++;
        }
        else{
            count[idx] = 1;
        }
    }
    raw_edge.clear();
    vector<int> hist(16,0);
    for (auto it=count.begin(); it != count.end(); it++){
        hist[it->second-1]++;
    }
    count.clear();
    uint64_t sum = 0;
    for (int i=15; i>=0; i--){
        sum += hist[i];
        cout << i+1 << ":\t" << hist[i]<<endl;
    }
    uint64_t block_num = (uint64_t) (max+1)/4;
    sum = block_num*block_num - sum;
    cout << "0:\t" << sum << endl;
	return 0;
}

int processRaw(char* mmap_ptr, size_t begin, size_t last,  vector<struct edge> &raw_edge){
	char* next_pos = nullptr;
	int max = 0;
    for (char* ptr=mmap_ptr+begin; ptr<mmap_ptr+last; ++ptr){
		for (;isblank(ptr[0]);++ptr);
		for (;ptr[0]=='#';++ptr){
			for(;ptr[0]!='\n'&&ptr[0]!='\r';++ptr);
			for(;isblank(ptr[0]);++ptr);
		}
		uint64_t src_idx = strtoull(ptr,&next_pos,10);
		if(next_pos==ptr) break;
		uint64_t dst_idx = strtoull(next_pos,&ptr,10);
		if(next_pos==ptr) break;
		for(;isblank(ptr[0]);++ptr);
		
        struct edge new_edge;
        new_edge.src = src_idx;
        new_edge.dst = dst_idx;
        raw_edge.push_back(new_edge);
        max = max > src_idx ? max : src_idx;
        max = max > dst_idx ? max : dst_idx;
	}
    return max;
}
