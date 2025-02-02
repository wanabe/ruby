# frozen_string_literal: true
require 'test/unit'
require 'rbconfig'
require 'io/wait'

class TestIOWaitInRactor < Test::Unit::TestCase
  ext = "/io/wait.#{RbConfig::CONFIG['DLEXT']}"
  IO_WAIT_PATH = $".find {|path| path.end_with?(ext)}.dup.freeze

  def test_ractor
    assert_in_out_err(%W[-r#{IO_WAIT_PATH}], <<-"end;", ["true"], [])
      $VERBOSE = nil
      r = Ractor.new do
        $stdout.equal?($stdout.wait_writable)
      end
      puts r.take
    end;
  end
end if defined? Ractor
