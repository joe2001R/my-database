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
        result = run_script(["select",".exit"])
        expect(result).to match_array(["db > ","db > 1 joestar"])
    end

end