describe 'database' do
    before do
        `rm -rf test.db`
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

    it 'displays a prompt until i exit' do
        result = run_script([".exit",])
        expect(result).to match_array(["db > ",])
    end

    it 'persists one record' do
        run_script(["insert 1 joestar",".exit"])
        result = run_script(["select 1",".exit"])
        expect(result).to match_array(["db > ","db > 1 joestar"])
    end

    it 'persists many records' do
        run_script(["insert 1 joestar","insert 2 jotaro","insert 3 joe",".exit"])
        result = run_script(["select 1 2 3",".exit"])
        expect(result).to match_array(["db > ","db > 1 joestar","2 jotaro","3 joe"])
    end

end