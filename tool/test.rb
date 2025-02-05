$:.clear
RUBYBIN = Ractor.make_shareable(File.readlink("/proc/#{$$}/exe"))
exedir = File.dirname(RUBYBIN)
$:.unshift exedir, "#{exedir}/.ext/common"
require "rbconfig"
$:.unshift "#{exedir}/.ext/#{RbConfig::CONFIG["arch"]}"

$: << "#{__dir__}/../lib"
$: << "#{__dir__}/lib"
$: << "#{__dir__}/../test"

Warning[:deprecated] = true

require 'stringio'

module Test
  module Unit
    module CoreAssertions
    end
    module Assertions
      INCLUDE_PATHS = Ractor.make_shareable($:.map { "-I#{_1}" })
      def assert_predicate(obj, meth, msg = nil, inverse: false)
        assert(obj.__send__(meth), msg, inverse:)
      end
      def assert_not_predicate(obj, meth, msg = nil)
        assert_predicate(obj, meth, msg, inverse: true)
      end
      alias_method :refute_predicate, :assert_not_predicate
      def assert(val, msg = nil, inverse: false, backtrace: caller)
        val = !val if inverse
        return if val
        raise RuntimeError, msg ? msg.to_s : "assert fail: #{val.inspect} is falsey", backtrace
      end
      def refute(val, msg = nil)
        assert(!val, msg)
      end
      def assert_equal(l, r, msg = nil)
        assert(l == r, msg || "#{l.inspect} != #{r.inspect}")
      end
      def assert_not_equal(l, r, msg = nil)
        assert(l != r, msg)
      end
      def assert_raise(*klasses)
        msg = klasses.pop unless klasses.last.is_a?(Module)
        begin
          yield
        rescue *klasses => e
          e
        else
          raise msg ? msg : "assert fail: not raise"
        end
      end
      def assert_raise_with_message(klass, pat, msg = nil)
        begin
          yield
        rescue klass => e
          raise msg || "assert fail: #{e.message} !~ #{pat.inspect}" unless pat === e.message
          return e
        else
          raise msg || "assert fail: not raise"
        end
      end
      def assert_instance_of(klass, obj, msg = nil)
        assert(obj.class == klass, msg)
      end
      def assert_nothing_raised(e = nil, msg = nil)
        yield
      end
      def assert_operator(l, op, r, msg = nil)
        assert(l.__send__(op, r), msg)
      end
      def assert_not_operator(l, op, r, msg = nil)
        assert(!l.__send__(op, r), msg)
      end
      def assert_same(l, r, msg = nil)
        assert(l.equal?(r), msg)
      end
      def assert_not_same(l, r, msg = nil)
        assert(!l.equal?(r), msg)
      end
      def assert_warn(pat, msg = nil, backtrace: caller, &)
        assert(pat === EnvUtil.verbose_warning(&), msg, backtrace:)
      end
      def assert_warning(pat, msg = nil, backtrace: caller, &)
        assert(pat === EnvUtil.verbose_warning(&), msg, backtrace:)
      end
      def assert_nil(obj, msg = nil)
        assert(obj.nil?, msg)
      end
      def assert_not_nil(obj, msg = nil)
        assert(!obj.nil?, msg)
      end
      alias_method :refute_nil, :assert_not_nil
      def assert_kind_of(klass, obj, msg = nil)
        assert(obj.is_a?(klass), msg)
      end
      def assert_match(pat, obj, msg = nil, inverse: false)
        pat = Regexp.new(Regexp.escape(pat)) if pat.is_a?(String)
        assert(pat =~ obj, msg, inverse:)
      end
      def assert_no_match(pat, obj, msg = nil)
        assert_match(pat, obj, msg, inverse: true)
      end
      def assert_not_match(pat, obj, msg = nil)
        assert_match(pat, obj, msg, inverse: true)
      end
      alias_method :refute_match, :assert_not_match
      def assert_include(set, obj, msg = nil, inverse: false)
        assert(set.include?(obj), msg, inverse:)
      end
      def assert_includes(set, obj, msg = nil, inverse: false)
        assert_include(set, obj, msg, inverse:)
      end
      def assert_not_include(set, obj, msg = nil)
        assert_include(set, obj, msg, inverse: true)
      end
      def assert_output(out_pat = nil, err_pat = nil)
        orig = [$stdout, $stderr]
        begin
          $stdout, $stderr = Array.new(2) { StringIO.new }
          yield
          assert(out_pat === $stdout.string)
          assert(err_pat === $stderr.string)
        ensure
          $stdout, $stderr = orig
        end
      end
      def assert_syntax_error(src, pat, msg = nil)
        e = assert_raise(SyntaxError, msg) do
          RubyVM::InstructionSequence.compile(src)
        end
        assert_match pat, e.message, msg
        e
      end
      def assert_empty(obj, msg = nil, inverse: false)
        assert(obj.empty?, msg, inverse:)
      end
      def assert_not_empty(obj, msg = nil)
        assert_empty(obj, msg, inverse: true)
      end
      def assert_send(argv, msg = nil, inverse: false)
        obj, *rest = argv
        assert(obj.__send__(*rest), msg, inverse:)
      end
      def assert_not_send(argv, msg = nil)
        assert_send(argv, msg, inverse: true)
      end
      def assert_in_delta(expect, val, delta = 0.001, msg = nil)
        assert ((expect - delta)..(expect + delta)) === val, msg
      end
      def assert_in_epsilon(expect, val, epsilon = 0.001, msg = nil)
        assert_in_delta(expect, val, [a.abs, b.abs].min * epsilon, msg)
      end
      def assert_block(msg = nil)
        assert yield, msg
      end
      def assert_true(val, msg = nil)
        assert val == true, msg
      end
      def assert_false(val, msg = nil)
        assert val == false, msg
      end
      def assert_is_minus_zero(val, msg = nil)
        assert z.zero? && z.polar.last > 0, msg
      end
      def assert_ractor(*)
        omit
      end
      def assert_no_memory_leak(*)
        omit
      end
      def assert_linear_performance(*)
        omit
      end

      def assert_in_out_err(argv, stdin = "", expect_out = nil, expect_err = nil, msg = nil, success: nil, **opt)
        out, err, stat = EnvUtil.invoke_ruby(argv, stdin, true, true, rubybin: RUBYBIN, **opt)
        out_ary = out.scan(/^.+/)
        err_ary = err.scan(/^.+/)
        if block_given?
          yield out_ary, err_ary, stat
        end
        unless success.nil?
          assert_equal(stat.success?, success)
        end
        assert_output_result(expect_out, out_ary)
        assert_output_result(expect_err, err_ary)
      end
      def assert_output_result(expect, ary)
        case expect
        when nil
          return
        when Array
          assert_equal(expect, ary)
        when Regexp
          assert_match(expect, ary.join(""))
        else
          raise "unexpect type: #{expect.class}"
        end
      end
      def assert_ruby_status(argv, stdin = "", msg = nil, **opt)
        assert_in_out_err(argv, stdin, success: true)
      end
      def assert_separately(argv, scr, **opt)
        scr = <<~EOS
          require "tool/test"
          include Test::Unit::Assertions
          Ractor.make_shareable(ARGV)
          begin
            Ractor.new do
              #{scr}
            end.take
          rescue
            $stderr.puts $!.message
            exit(1)
          end
        EOS

        assert_normal_exit(scr, argv: ["-I#{__dir__}/..", *INCLUDE_PATHS, *argv])
      end
      def assert_normal_exit(scr, msg = nil, argv: nil)
        assert_in_out_err(["--disable-gems", *argv], scr) do |out_ary, err_ary, stat|
          assert(stat.success?, msg || err_ary.join(""))
        end
      end
      def assert_join_threads(threads)
        threads.map(&:value)
      end

      def omit(o = nil)
        return if block_given?
        raise Test::Unit::TestCase::OmitTest, o
      end
      def message(*)
        nil
      end
      def windows?
        false
      end
    end

    class TestCase
      include Assertions

      @children = [].freeze

      def setup
      end
      def teardown
      end

      class OmitTest < StandardError
        attr_reader :obj

        def initialize(obj)
          @obj = obj
        end
      end

      class << self
        attr_accessor :children

        def windows?
          false
        end

        def inherited(child)
          path = caller.first[/(^.*?):\d+:in/, 1]
          Test::Unit::TestCase.children = Ractor.make_shareable([*Test::Unit::TestCase.children, [child, path]])
        end
      end
    end
  end
end

require "envutil"
require "tempfile"
require "objspace"
# omit ractor-unsafe method
def ObjectSpace.memsize_of(*)
  raise Test::Unit::TestCase::OmitTest, "ObjectSpace.memsize_of"
end

return if $0 != __FILE__

if ARGV[-2] == "-e"
  _, pat = ARGV.pop(2)
  PAT = Regexp.new(pat).freeze
else
  PAT = nil
end

tests = Dir.glob("test/**/test_*.rb")
unless ARGV.empty?
  given_tests = ARGV.map do |path|
    path = "#{path}/*" if File.directory?(path)
    Dir.glob(path)
  end.flatten
  tests &= given_tests
end
tests.each do |path|
  next if path =~ %r{
  test/-ext-/thread
  |test/fileutils/test_fileutils.rb
  |test/fiber/test_mutex.rb
  |test/fiber/test_scheduler.rb
  |test/net/http/test_httpresponse.rb
  |test/ruby/test_autoload.rb
  |test/ruby/test_exception.rb
  |test/ruby/test_io.rb
  |test/ruby/test_require.rb

  |test/fiber/test_ractor.rb
  }x
  puts "load #{path}"
  load path
end

T0 = Ractor.make_shareable(Time.now)
def log(obj)
  ractor_id = Ractor.current.to_s[/#(\d+)/, 1].to_i
  $stderr.puts "[%4d %10.4f] %s" % [ractor_id, Time.now - T0, obj]
end

total_success_count = total_omit_count = total_failure_count = 0

Test::Unit::TestCase.children.sort_by { _1.first.name }.each do |(test, path)|
  r = Ractor.new(test, path) do |test, path|
    success_count = omit_count = failure_count = 0
    log "#{test} #{path}"
    testcases = test.instance_methods.select { _1.start_with?("test_") }.sort
    testcases.keep_if { PAT =~ _1 } if PAT
    testcases.each do |testcase|
      testobj = test.new
      begin
        testobj.setup
        testobj.__send__(testcase)
        log "  #{testcase} OK"
        success_count += 1
      rescue Test::Unit::TestCase::OmitTest => e
        omit_count += 1
      rescue StandardError, Ractor::Error, LoadError => e
        log "  #{testcase} fail(#{e.class}): #{e} at #{e.backtrace&.first}"
        failure_count += 1
      ensure
        begin
          testobj.teardown
        rescue StandardError
        end
      end
    end
    [success_count, omit_count, failure_count]
  end
  counts = r.take
  total_success_count += counts[0]
  total_omit_count += counts[1]
  total_failure_count += counts[2]
  log "#{test} #{path} result: #{counts}"
  log "progress: {success: #{total_success_count}, omit: #{total_omit_count}, failure: #{total_failure_count}}"
end
