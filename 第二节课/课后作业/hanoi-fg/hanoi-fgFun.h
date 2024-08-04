
#include <stack>
using namespace std;

int sum = 0;
typedef struct Element {
    int n;
    char type;
}Element;

void fg_no_re(int n, char type) {
    sum = 0;
    stack<Element> stack;
    stack.push(Element{n, type});
    while(!stack.empty()) {
        auto e = stack.top();
        stack.pop();
        n = e.n; type = e.type;
        if(n <= 1)  { //base case
            sum += 1;
            continue;
        }

        if(type == 'f') {
            stack.push(Element{n - 1, type});
            stack.push(Element{n - 2, 'g'});
        } else {
            stack.push(Element{n - 1, type});
            stack.push(Element{n + 1, 'f'});
        }
    }
}
