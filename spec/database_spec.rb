describe 'database' do
    before do
        `rm -f test.db`
    end

    after do
        `rm -f test.db`
    end

    def run_script(commands)
        raw_output = nil
        IO.popen("./db test.db","r+") do |pipe|
            commands.each do |command|
                begin
                    pipe.puts command
                rescue Errno::EPIPE
                    break
                end
            end

            pipe.close_write

            raw_output=pipe.gets(nil)
        end
        raw_output.split("\n")
    end

    it 'persists one record' do
        run_script(["insert 1 joestar",".exit"])
        result = run_script(["select 1",".exit"])
        expect(result).to match_array(["db > ","db > 1 joestar"])
    end

    it 'persists many records' do
        run_script(["insert 100 joestar","insert 200 jotaro","insert 3 joe",".exit"])
        result = run_script(["select 100 200 3",".exit"])
        expect(result).to match_array(["db > ","db > 100 joestar","200 jotaro","3 joe"])
    end

    it 'prints every record in order' do
        run_script(["insert 100 joestar","insert 200 jotaro","insert 3 joe","insert 4 jojo",".exit"])
        result = run_script(["select *",".exit"])
        expect(result).to match_array(["db > ","db > 3 joe","4 jojo" ,"100 joestar","200 jotaro"])
        
    end

    it 'persists even more records' do
        run_script(["insert 100 joestar","insert 200 jotaro","insert 3 joe","insert 500 joe1","insert 600 joe2","insert 700 joe3",".exit"])
        result = run_script(["select *",".exit"])
        expect(result).to match_array(["db > ", "db > 3 joe" ,"100 joestar","200 jotaro","500 joe1","600 joe2","700 joe3"])
    end
    
    it 'persists much more records' do
        
        insert_input = []
        
        num_records=203

        for i in 1..num_records do
            insert_input.push("insert #{i} example#{i}")
        end

        insert_input.push(".exit")

        run_script(insert_input)

        expected_output = ["db > ","db > 1 example1"]

        for i in 2..num_records do
            expected_output.push("#{i} example#{i}")
        end
        result = run_script(["select *",".exit"])

        `mv test.db joetest.db`

        expect(result).to match_array(expected_output)
    end

    it 'persists records correctly regardless of order of insert' do
        insert_input = 
        [
            "insert 10 joe",
            "insert 100 joe",
            "insert 50 joe",
            "insert 40 joe",
            "insert 20 joe",
            "insert 90 joe",
            "insert 15 joe",
            "insert 18 joe",
            "insert 4 joe",
            ".exit"
        ]

        run_script(insert_input)
        result = run_script([".btree",".exit"])

        expected_output = 
        [
        "db > internal node: num of nodes is 3",
        "    child0's key is 15",
        "    leaf node: num of records is 3",
        "        key:4, value:joe",
        "        key:10, value:joe",
        "        key:15, value:joe",
        "    child1's key is 40",
        "    leaf node: num of records is 3",
        "        key:18, value:joe",
        "        key:20, value:joe",
        "        key:40, value:joe",
        "    right child",
        "    leaf node: num of records is 3",
        "        key:50, value:joe",
        "        key:90, value:joe",
        "        key:100, value:joe",
        "db > "
        ]

        expect(result).to match_array(expected_output)
    end

end