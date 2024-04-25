# tv_lab3

## How to run

- Clone repo.
    ```sh
    got clone https://github.com/ahmedXDR/tv_lab3.git

    cd tv_lab3
    ```

- Create rootfs archive
    ```sh
    docker build -t ubuntu_sysbench .
    docker create --name sysbench-container ubuntu_sysbench
    docker export sysbench-container -o rootfs.tar
    ```
    
- Compile main script
    ```sh
    g++ main.cpp -o main  
    ```
- Make excutable and run main script
    ```sh
    chmod +x main
    ./main
    ```
- To run benchmark script run the following command inside the conatiner
    ```sh
    python3 benchmark.py
    ```
