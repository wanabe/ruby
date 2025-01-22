$: << "#{__dir__}/../lib"
$: << "#{__dir__}/lib"

require 'stringio'

module Test
  module Unit
    class TestCase
      @children = [].freeze

      class OmitTest < StandardError
        attr_reader :obj

        def initialize(obj)
          @obj = obj
        end
      end

      class << self
        attr_reader :children

        def inherited(child)
          return if self != Test::Unit::TestCase
          @children = Ractor.make_shareable([*@children, child])
        end
      end

      def setup
      end

      def assert_predicate(obj, meth)
        assert(obj.__send__(meth))
      end
      def assert_not_predicate(obj, meth)
        assert(!obj.__send__(meth))
      end
      def assert(val, msg = nil, inverse: false)
        val = !val if inverse
        return if val
        raise msg ? msg.to_s : "assert fail: #{val} is falsey"
      end
      def refute(val, msg = nil)
        assert(!val, msg)
      end
      def assert_equal(l, r, msg = nil)
        assert(l == r, msg || "#{l} != #{r}")
      end
      def assert_not_equal(l, r, msg = nil)
        assert(l != r, msg)
      end
      def assert_raise(klass, msg = nil)
        begin
          yield
        rescue klass => e
          e
        else
          raise msg ? msg : "assert fail: not raise"
        end
      end
      def assert_raise_with_message(klass, pat, msg = nil)
        begin
          yield
        rescue klass => e
          raise msg || "assert fail: #{e.message} !~ #{pat}" unless pat === e.message
        else
          raise msg || "assert fail: not raise"
        end
      end
      def assert_instance_of(klass, obj)
        assert(obj.class == klass)
      end
      def assert_nothing_raised(e = nil, msg = nil)
        yield
      end
      def assert_operator(l, op, r)
        assert(l.__send__(op, r))
      end
      def assert_not_operator(l, op, r)
        assert(!l.__send__(op, r))
      end
      def assert_same(l, r, msg = nil)
        assert(l.equal?(r), msg)
      end
      def assert_not_same(l, r, msg = nil)
        assert(!l.equal?(r), msg)
      end
      def assert_warn(pat, &)
        assert(EnvUtil.verbose_warning(&) =~ pat)
      end
      def assert_warning(pat, &)
        assert(EnvUtil.verbose_warning(&) =~ pat)
      end
      def assert_nil(obj, msg = nil)
        assert(obj.nil?, msg)
      end
      def assert_not_nil(obj, msg = nil)
        assert(!obj.nil?, msg)
      end
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
      def assert_syntax_error(src, pat)
        e = assert_raise(SyntaxError, mesg) do
          RubyVM::InstructionSequence.compile(src)
        end
        assert_match pat, e.message
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
      def assert_is_minus_zero(val)
        assert z.zero? && z.polar.last > 0
      end

      def omit(o = nil)
        return if block_given?
        raise o if o.is_a?(StandardError)
        raise OmitTest, o
      end
    end
  end
end

require "envutil"
tests = ARGV 
if tests.empty?
  tests = Dir.glob("test/**/test_*.rb")
end
tests.each do |path|
  if path =~ %r{
    test/-ext-/|
    test/mkmf/|
    test/net/http/test_https_proxy|
    test/psych/|
    test/resolv/test_dns|
    test/ruby/test_autoload.rb|
    test/ruby/test_call.rb|
    test/ruby/test_dir_m17n.rb|
    test/ruby/test_exception.rb|
    test/ruby/test_file.rb|
    test/ruby/test_file_exhaustive.rb|
    test/ruby/test_io.rb|
    test/ruby/test_keyword.rb|
    test/ruby/test_memory_view.rb|
    test/ruby/test_require.rb|
    test/ruby/test_time_tz.rb|
    test/rubygems/|
    test/test_bundled_gems.rb|
    test/test_extlibs.rb
    }x
    puts "skip #{path}"
    next
  end
  puts "load #{path}"
  load path
end

T0 = Ractor.make_shareable(Time.now)
def log(obj)
  $stderr.puts "[#{Ractor.current} #{"%10.4f" % (Time.now - T0)}] #{obj}"
end

total_success_count = total_omit_count = total_failure_count = 0

Test::Unit::TestCase.children.each do |test|
  r = Ractor.new(test) do |test|
    success_count = omit_count = failure_count = 0
    log test
    testcases = test.instance_methods.select { _1.start_with?("test_") }.sort
    testcases.each do |testcase|
      log "  #{testcase}"
      testobj = test.new
      begin
        testobj.setup
        testobj.__send__(testcase)
        success_count += 1
      rescue Test::Unit::TestCase::OmitTest => e
        log "    omit: #{e.obj}"
        omit_count += 1
      rescue StandardError, LoadError => e
        log "    fail: #{e} at #{e.backtrace&.first}"
        failure_count += 1
      end
    end
    [success_count, omit_count, failure_count]
  end
  counts = r.take
  total_success_count += counts[0]
  total_omit_count += counts[1]
  total_failure_count += counts[2]
  log "#{test} result: #{counts}"
  log "progress: {success: #{total_success_count}, omit: #{total_omit_count}, failure: #{total_failure_count}}"
end
