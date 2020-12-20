#include <iostream>
#include <getopt.h>
#include <cmath>
#include <omp.h>
#include "csr.hpp"
#include "sellcs.hpp"

int main(int argc, char *const *argv) {

    int32_t root = 0, chunk_size=8, sigma=1,SCALE=-1,EDGEFACTOR=-1, mode=1;
    std::string file_path = "none";
    if(argc < 2) {
        std::cout << "Error. Need filename with graph." << std::endl;
        exit(0);
    }

    // parse arguments
    int opt;
    while ((opt = getopt(argc,argv,":C:s:S:E:r:f:m:")) != -1)
    {
        switch (opt)
        {
            case 'C':
                chunk_size = std::atoi(optarg);
                break;
            case 's':
                sigma = std::atoi(optarg);            
                break;
            case 'S':
                SCALE = std::atoi(optarg);
                break;
            case 'E':
                EDGEFACTOR = std::atoi(optarg);
                break;
            case 'f':
                file_path = optarg;
                break;
            case 'r':
                root = std::atoi(optarg);
                break;
            case 'm':
                mode = std::atoi(optarg);
                break;
            default:
                std::cout << "Unrecognized flag: " << opt << "\nTerminating!" << std::endl;
                exit(0);
                break;
        }
    }

    // check arguments
    if(SCALE == -1) {
        std::cout << "ERROR! SCALE must be supplied with: -S X, where X is 2-logarithm of number of vertices." << std::endl;
        exit(0);
    }
    if(EDGEFACTOR == -1) {
        std::cout << "ERROR! EDGEFACTOR must be supplied with: -E X, where X is #edges / #vertices." << std::endl;
        exit(0);
    }
    if(root < 0 || root >= pow(2,SCALE)) {
        std::cout << "ERROR! root = " << root << " is not valid root. Root must be 0 <= root < 2^SCALE. Supply root with: -r X" << std::endl;
        exit(0);
    }
    if(chunk_size != 4 && chunk_size != 8) {
        std::cout << "ERROR! chunk size must be =4 or =8. Modify chunk size with: -C X" << std::endl;
        exit(0);
    }
    if(sigma < 0 || sigma > SCALE) {
        std::cout << "ERROR! sigma must be 0 <= sigma <= SCALE. Modify sigma with: -s X" << std::endl;
        exit(0);
    }
    if(file_path == "none") {
        std::cout << "ERROR! file path containing edges must be specified! Use: -f path/to/file/" << std::endl; 
        exit(0);
    }
    if(mode < 1 || mode > 3) {
        std::cout << "ERROR! Mode must be one of the following:\n" << std::endl;
        std::cout << "mode = 1: CSR only.\n" << std::endl;
        std::cout << "mode = 2: SELL-C-Sigma only.\n" << std::endl;
        std::cout << "mode = 3: CSR and SELL-C-Sigma.\n" << std::endl;
        std::cout << "supply mode with flag m: -m <mode> \n" << std::endl;
        exit(0);
    }

    // timer indices (for SELL-C-SIGMA) have following defintions:
        // timer[0] = time to read graph to buffer
        // timer[1] = time to process edges in buffer
        // timer[2] = time to sort edges
        // timer[3] = time to create SELL-C-SIGMA datastructure

    double CSR_timer[5], SELLCS_timer[5];
    std::vector<int32_t> CSR_res, SELLCS_res;
    if(mode & 1) 
    {
        csr_graph csr_g;
        read_csr_graph_from_file(file_path, csr_g, SCALE, EDGEFACTOR);
        CSR_res = csr_bfs(csr_g, root);
        delete_csr(csr_g);
    }

    // BFS with SELL-C-SIGMA datastructure
    if(mode & 2) 
    {
        sellcs sellcs_g;
        sellcs_g.C = chunk_size;
        sellcs_g.sigma = pow(2,sigma);
        read_sellcs_graph_from_file(file_path, sellcs_g, SCALE, EDGEFACTOR, SELLCS_timer);
        SELLCS_res = sellcs_bfs(sellcs_g, root);
        permutate_solution(SELLCS_res, sellcs_g);
        delete_sellcs(sellcs_g);

        // print times with SELL-C-SIGMA   
        std::cout << "--SELL-C-SIGMA--" << std::endl;
        std::cout <<  "Buffer read time: \t\t" << SELLCS_timer[0] << " s"  << std::endl;
        std::cout << "Process edges time: \t\t" << SELLCS_timer[1] << " s"  << std::endl;
        std::cout << "Sort vertices time: \t\t" << SELLCS_timer[2] << " s"  << std::endl;
        std::cout << "Create SELL-C-SIGMA time: \t" << SELLCS_timer[3] << " s"  << std::endl;
    }

    if((mode & 1) && (mode & 2))
    {
        int32_t nverts = pow(2,SCALE), same=1;
        for(int i = 0; i < nverts; ++i)
        {
            if(CSR_res[i] != SELLCS_res[i]) 
            {
                std::cout << "Different solutions! " << CSR_res[i] << " != " << SELLCS_res[i] << std::endl;
                same = 0;
            }
        }
        std::cout << "Solutions are " << (same ? "the same!" : "not the same!") << std::endl;
    }
    return 0;
}
