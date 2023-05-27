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

end