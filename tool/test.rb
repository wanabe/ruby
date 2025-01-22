$: << "#{__dir__}/../lib"
$: << "#{__dir__}/lib"

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
        assert obj.__send__(meth)
      end
      def assert_not_predicate(obj, meth)
        assert !obj.__send__(meth)
      end
      def assert(val, msg = nil)
        return if val
        raise msg ? msg.to_s : "assert fail: #{val} is falsey"
      end
      def refute(val, msg = nil)
        assert !val, msg
      end
      def assert_equal(l, r, msg = nil)
        assert l == r, msg || "#{l} != #{r}"
      end
      def assert_not_equal(l, r, msg = nil)
        assert l != r, msg
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
          raise msg || "assert fail: #{e.message} !~ #{pat}" if e.message !~ pat
        else
          raise msg || "assert fail: not raise"
        end
      end
      def assert_instance_of(klass, obj)
        assert obj.class == klass
      end
      def assert_nothing_raised(e = nil, msg = nil)
        yield
      end
      def assert_operator(l, op, r)
        assert l.__send__(op, r)
      end
      def assert_not_operator(l, op, r)
        assert !l.__send__(op, r)
      end
      def assert_same(l, r, msg = nil)
        assert l.equal?(r), msg
      end
      def assert_not_match(pat, obj, msg = nil)
        assert obj !~ pat, msg
      end
      def assert_warn(pat, &)
        assert EnvUtil.verbose_warning(&) =~ pat
      end
      def assert_warning(pat, &)
        assert EnvUtil.verbose_warning(&) =~ pat
      end
      def assert_nil(obj, msg = nil)
        assert obj.nil?, msg
      end
      def assert_kind_of(klass, obj, msg = nil)
        assert obj.is_a?(klass), msg
      end
      def assert_match(pat, obj, msg = nil)
        assert pat =~ obj, msg
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
ARGV.each do |script|
  load script
end

def log(obj)
  $stderr.puts "[#{Ractor.current}] #{obj}"
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
