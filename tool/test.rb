$: << "#{__dir__}/../lib"
$: << "#{__dir__}/lib"
$: << "#{__dir__}/../test"

require 'stringio'

module Test
  module Unit
    module CoreAssertions
    end
    module Assertions
      def assert_predicate(obj, meth, msg = nil, inverse: false)
        assert(obj.__send__(meth), msg, inverse:)
      end
      def assert_not_predicate(obj, meth, msg = nil)
        assert_predicate(obj, meth, msg, inverse: true)
      end
      alias_method :refute_predicate, :assert_not_predicate
      def assert(val, msg = nil, inverse: false)
        val = !val if inverse
        return if val
        raise msg ? msg.to_s : "assert fail: #{val.inspect} is falsey"
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
        msg = klasses.pop unless klasses.last.is_a?(Class)
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
      def assert_warn(pat, msg = nil, &)
        assert(EnvUtil.verbose_warning(&) =~ pat, msg)
      end
      def assert_warning(pat, msg = nil, &)
        assert(EnvUtil.verbose_warning(&) =~ pat, msg)
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
        assert(pat =~ obj, msg, inverse:)
      end
      def assert_no_match(pat, obj, msg = nil)
        assert_match(pat, obj, msg, inverse: true)
      end
      def assert_not_match(pat, obj, msg = nil)
        assert_match(pat, obj, msg, inverse: true)
      end
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
        e = assert_raise(SyntaxError, mesg) do
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
      def assert_in_out_err(argv, stdin, out_ary, err_ary, msg = nil, **opt)
        out, err, = EnvUtil.invoke_ruby(argv, stdin, true, true, **opt)
        raise NotImplementError if block_given?
        assert_equal(out_ary, out.scan(/^.+/))
        assert_equal(err_ary, err.scan(/^.+/))
      end
      def assert_join_threads(threads)
        threads.map(&:value)
      end

      def omit(o = nil)
        return if block_given?
        raise o if o.is_a?(StandardError)
        raise Test::Unit::TestCase::OmitTest, o
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
        log "  #{testcase} fail: #{e} at #{e.backtrace&.first}"
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
