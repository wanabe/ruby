$: << "#{__dir__}/../lib"
$: << "#{__dir__}/lib"

module Test
  module Unit
    class TestCase
      @children = [].freeze

      class << self
        attr_reader :children

        def inherited(child)
          return if self != Test::Unit::TestCase
          @children = Ractor.make_shareable([*@children, child])
        end
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
        assert l == r, msg
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
      def assert_nothing_raised(e, msg = nil)
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
    end
  end
end

ARGV.each do |script|
  load script
end
require "envutil"

def log(obj)
  $stderr.puts "[#{Ractor.current}] #{obj}"
end

Test::Unit::TestCase.children.each do |test|
  Ractor.new(test) do |test|
    log test
    testcases = test.instance_methods.select { _1.start_with?("test_") }
    testcases.each do |testcase|
      log "  #{testcase}"
      test.new.__send__(testcase)
    end
  end.take
end
